// File: src/core/artifacts/ArtifactFactory.cpp
/**
 * @file src/core/artifacts/ArtifactFactory.cpp
 * @brief Implements the ArtifactFactory for creating and saving output artifacts.
 */
#include "ArtifactFactory.hpp"
#include "../graphics/PlotBoundsCalculator.hpp"
#include "../graphics/PlotDataGenerator.hpp"
#include "../graphics/PlotOrchestrator.hpp"
#include "../graphics/RenderContext.hpp"
#include "../graphics/Constants.hpp"
#include "../utils/OutputFilenameGenerator.hpp"
#include "../utils/PlotTitleGenerator.hpp"
#include "../utils/Formatters.hpp"
#include "../io/OutputWriter.hpp"
#include "../utils/Constants.hpp"          // Incluido previamente, ahora también para LOG_OUTPUT_FILENAME
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <fstream>
#include <libintl.h>
#include <cstring>
#include <mutex>
#include <set>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace { // Anonymous namespace for internal helpers

// *** INICIO CÓDIGO MOVIDO Y ADAPTADO DESDE ChartGenerator.cpp ***
/**
 * @brief Internal helper to create the Cairo surface containing the test chart image.
 * Contains the core drawing logic previously in ChartGenerator.cpp's helper.
 * @param opts The validated chart generation parameters.
 * @param log_stream Stream for logging potential errors during creation.
 * @return A pointer to the created cairo_surface_t on success, or nullptr on failure.
 */
cairo_surface_t* CreateChartSurfaceInternal(const ChartGeneratorOptions& opts, std::ostream& log_stream) {
    // Input validation
    if (opts.R < 0 || opts.R > 255 || opts.G < 0 || opts.G > 255 || opts.B < 0 || opts.B > 255 ||
        opts.invgamma <= 0.0 || opts.dim_x <= 0 || opts.aspect_w <= 0 || opts.aspect_h <= 0 ||
        opts.patches_m <= 0 || opts.patches_n <= 0)
    {
        log_stream << _("Error: Invalid chart parameters provided to CreateChartSurfaceInternal.") << std::endl;
        return nullptr;
    }

    // Calculate dimensions
    const int DIMX = opts.dim_x;
    const int DIMY = static_cast<int>(round(static_cast<double>(DIMX) * opts.aspect_h / opts.aspect_w));
    const int NROWS = opts.patches_m;
    const int NCOLS = opts.patches_n;

    // Create Cairo surface and context
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, DIMX, DIMY);
    if (!surface || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
         log_stream << _("Error: Failed to create Cairo image surface for chart.") << std::endl;
         if(surface) cairo_surface_destroy(surface);
         return nullptr;
    }
    cairo_t* cr = cairo_create(surface);
    if (!cr || cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("Error: Failed to create Cairo context for chart.") << std::endl;
        cairo_surface_destroy(surface);
        if(cr) cairo_destroy(cr);
        return nullptr;
    }

    // Drawing constants
    constexpr double ALPHA = 0.8;    // Effective area factor
    constexpr double RGBMAX = 255.0; // Normalization factor for color values

    // Draw black background
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    // Calculate effective canvas dimensions and offsets
    const double DIMXc = DIMX * ALPHA;
    const double DIMYc = DIMY * ALPHA;
    const double OFFSETX = (DIMX - DIMXc) / 2.0;
    const double OFFSETY = (DIMY - DIMYc) / 2.0;

    // --- Draw Patches ---
    const double patch_width = DIMXc / (NCOLS + 1);
    const double patch_height = DIMYc / (NROWS + 1);
    int patch_index = 0;
    for (int row = 0; row < NROWS; ++row) {
        for (int col = 0; col < NCOLS; ++col) {
            // Calculate intensity (0 to 1) with gamma correction
            double t = (NCOLS * NROWS > 1) ? static_cast<double>(patch_index) / (NCOLS * NROWS - 1) : 0.0;
            double intensity = std::pow(1.0 - t, opts.invgamma);
            // Calculate patch position (top-left corner)
            double x = (col * patch_width) + OFFSETX + (patch_width / 2.0);
            double y = (row * patch_height) + OFFSETY + (patch_height / 2.0);
            // Set color and draw rectangle
            cairo_set_source_rgb(cr, (intensity * opts.R) / RGBMAX,
                                   (intensity * opts.G) / RGBMAX,
                                   (intensity * opts.B) / RGBMAX);
            cairo_rectangle(cr, x, y, patch_width, patch_height);
            cairo_fill(cr);
            patch_index++;
        }
    }

    // --- Draw Border and Corner Markers ---
    // Corner coordinates
    std::vector<double> x0 = {OFFSETX, OFFSETX, DIMX - OFFSETX, DIMX - OFFSETX};
    std::vector<double> y0 = {OFFSETY, DIMY - OFFSETY, DIMY - OFFSETY, OFFSETY};
    // Radius calculation (1% of image diagonal)
    const double diag = std::sqrt(static_cast<double>(DIMX * DIMX + DIMY * DIMY));
    const double RADIUS = diag * 0.01;

    // Draw blue border lines
    cairo_set_source_rgb(cr, 0.0, 0.0, 191.0 / 255.0); // Blue color
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, x0[0], y0[0]);
    cairo_line_to(cr, x0[1], y0[1]);
    cairo_line_to(cr, x0[2], y0[2]);
    cairo_line_to(cr, x0[3], y0[3]);
    cairo_close_path(cr);
    cairo_stroke(cr);

    // Draw white corner circles
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White color
    for (size_t i = 0; i < 4; ++i) {
        cairo_arc(cr, x0[i], y0[i], RADIUS, 0, 2 * M_PI);
        cairo_fill(cr);
    }

    // Clean up Cairo context
    cairo_destroy(cr);

    // Check for errors during drawing
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("Error: Cairo drawing operations failed for chart surface.") << std::endl;
        cairo_surface_destroy(surface);
        return nullptr;
    }

    return surface; // Return the completed surface
}
// *** FIN CÓDIGO MOVIDO Y ADAPTADO ***


/**
 * @brief Internal helper to generate and save a plot artifact (PNG, PDF, or SVG).
 * This function encapsulates the core plotting and saving logic previously in Plotting.cpp.
 * @param output_filename The full path where the plot file will be saved.
 * @param title The main title for the plot.
 * @param curves_to_plot The curve data specific to this plot.
 * @param results_to_plot The dynamic range results specific to this plot (for labels).
 * @param reporting_params Parameters controlling plot rendering details.
 * @param bounds Pre-calculated axis boundaries for the plot.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the output filename on success, std::nullopt on failure.
 */
std::optional<std::string> GeneratePlotInternal(
    const std::string& output_filename,
    const std::string& title,
    const std::vector<CurveData>& curves_to_plot,
    const std::vector<DynamicRangeResult>& results_to_plot,
    const ReportingParameters& reporting_params,
    const std::map<std::string, double>& bounds,
    std::ostream& log_stream)
{
    // Determine render context based on base dimensions and GUI scaling (if applicable)
    // For factory, we assume file output, so no GUI scaling.
    const auto render_ctx = DynaRange::Graphics::RenderContext{
        DynaRange::Graphics::Constants::PlotDefs::BASE_WIDTH,
        DynaRange::Graphics::Constants::PlotDefs::BASE_HEIGHT
    };

    // Determine scaling for vector formats
    bool is_vector = (reporting_params.plot_format == DynaRange::Graphics::Constants::PlotOutputFormat::SVG ||
                      reporting_params.plot_format == DynaRange::Graphics::Constants::PlotOutputFormat::PDF);
    double scale = is_vector ? DynaRange::Graphics::Constants::VECTOR_PLOT_SCALE_FACTOR : 1.0;
    int width = static_cast<int>(render_ctx.base_width * scale);
    int height = static_cast<int>(render_ctx.base_height * scale);

    // Create Cairo surface based on format
    cairo_surface_t *surface = nullptr;
    switch (reporting_params.plot_format) {
        case DynaRange::Graphics::Constants::PlotOutputFormat::SVG:
            surface = cairo_svg_surface_create(output_filename.c_str(), width, height);
            break;
        case DynaRange::Graphics::Constants::PlotOutputFormat::PDF:
            surface = cairo_pdf_surface_create(output_filename.c_str(), width, height);
            break;
        case DynaRange::Graphics::Constants::PlotOutputFormat::PNG:
        default: // Default to PNG
            surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
            break;
    }

    // Check surface creation
    if (!surface || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("  - Error: Failed to create cairo surface for plot \"") << title << "\"." << std::endl;
        if (surface) cairo_surface_destroy(surface);
        return std::nullopt;
    }

    // Create Cairo context
    cairo_t *cr = cairo_create(surface);
    if (!cr || cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("  - Error: Failed to create cairo context for plot \"") << title << "\"." << std::endl;
        cairo_surface_destroy(surface);
        if (cr) cairo_destroy(cr);
        return std::nullopt;
    }

    // Apply scaling if needed for vector formats
    if (scale != 1.0) {
        cairo_scale(cr, scale, scale);
    }

    // --- Perform the actual drawing using the central orchestrator ---
    // Prepare curve points if not already done (caller should ideally do this)
    // Assuming curves_to_plot already have curve_points generated.
    DynaRange::Graphics::DrawPlotToCairoContext(
        cr, render_ctx, curves_to_plot, results_to_plot, title, reporting_params, bounds);

    // --- Save the surface to file ---
    bool success = false;
    switch (reporting_params.plot_format) {
        case DynaRange::Graphics::Constants::PlotOutputFormat::SVG:
        case DynaRange::Graphics::Constants::PlotOutputFormat::PDF:
            // For vector formats, just flush the surface. OutputWriter isn't needed.
            cairo_surface_flush(surface);
            success = (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
            if (success) {
                 // Log success directly here as OutputWriter isn't involved
                log_stream << _("  - Info: Plot saved to: ") << output_filename << std::endl;
            }
            break;
        case DynaRange::Graphics::Constants::PlotOutputFormat::PNG:
        default:
             // Use OutputWriter for PNG saving
            success = OutputWriter::WritePng(surface, output_filename, log_stream);
            break;
    }

    // Clean up Cairo resources
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // Return the filename if successful
    if (success) {
        return output_filename;
    }
    // Log failure if not already logged by WritePng
    if (reporting_params.plot_format != DynaRange::Graphics::Constants::PlotOutputFormat::PNG) {
         log_stream << _("  - Error: Failed to write plot to file: ") << output_filename << std::endl;
    }
    return std::nullopt;
}

} // end anonymous namespace


namespace ArtifactFactory {

// --- CSV Report ---
std::optional<fs::path> CreateCsvReport(
    const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateCsvFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Format data
    auto sorted_rows = Formatters::FlattenAndSortResults(results);
    // 4. Write file
    if (OutputWriter::WriteCsv(sorted_rows, full_path, log_stream)) {
        return full_path;
    }
    return std::nullopt;
}

// --- Summary Plot ---
std::optional<fs::path> CreateSummaryPlot(
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // --- START: Logic moved/adapted from GenerateSummaryPlot ---
    if (!reporting_params.generate_plot) {
        // log_stream << "\n" << _("Plot generation skipped as per user request.") << std::endl; // Logged elsewhere maybe?
        return std::nullopt; // Plot not requested
    }
    if (curves.empty()) {
        log_stream << _("  - Warning: Skipping summary plot generation due to no curve data.") << std::endl;
        return std::nullopt; // No data to plot
    }

    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateSummaryPlotFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Generate title
    std::string title = PlotTitleGenerator::GenerateSummaryTitle(ctx);

    // 4. Prepare curve points and calculate bounds
    std::vector<CurveData> curves_with_points = curves; // Make a mutable copy
    for (auto& curve : curves_with_points) {
        // Generate plot points if they don't exist
        // (Ideally, this should happen earlier, but ensures safety here)
        if (curve.curve_points.empty()) {
             curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
        }
    }
    const auto bounds = DynaRange::Graphics::CalculateGlobalBounds(curves_with_points);

    // 5. Call the internal plotting helper
    std::optional<std::string> result_path_str = GeneratePlotInternal(
        full_path.string(), title, curves_with_points, results, reporting_params, bounds, log_stream);
    // --- END: Logic moved/adapted ---

    if (result_path_str) {
        return fs::path(*result_path_str);
    }
    return std::nullopt;
}

// --- Individual Plot ---
std::optional<fs::path> CreateIndividualPlot(
    const std::vector<CurveData>& curves_for_file,
    const std::vector<DynamicRangeResult>& results_for_file,
    const OutputNamingContext& ctx, // Must contain ISO info
    const ReportingParameters& reporting_params,
    const std::map<std::string, double>& global_bounds, // Use pre-calculated global bounds
    const PathManager& paths,
    std::ostream& log_stream)
{
    // --- START: New implementation ---
    if (!reporting_params.generate_plot || !reporting_params.generate_individual_plots) {
        return std::nullopt; // Plots not requested
    }
     if (curves_for_file.empty()) {
        // log_stream << _("  - Warning: Skipping individual plot generation due to no curve data.") << std::endl;
        return std::nullopt; // No data for this specific file
    }

    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateIndividualPlotFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Generate title
    std::string title = PlotTitleGenerator::GenerateIndividualTitle(ctx);
    // Fallback title if ISO was missing or title generation failed
    if (title.empty()) {
        title = fs::path(curves_for_file[0].filename).filename().string();
    }

    // 4. Prepare curve points (ensure they exist)
    std::vector<CurveData> curves_with_points = curves_for_file; // Mutable copy
    for (auto& curve : curves_with_points) {
        if (curve.curve_points.empty()) {
            curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
        }
    }
    // Note: We use the pre-calculated global_bounds passed as argument for consistency

    // 5. Call the internal plotting helper
    std::optional<std::string> result_path_str = GeneratePlotInternal(
        full_path.string(), title, curves_with_points, results_for_file, reporting_params, global_bounds, log_stream);
    // --- END: New implementation ---

    if (result_path_str) {
        return fs::path(*result_path_str);
    }
    return std::nullopt;
}


// --- Print Patches Debug Image ---
std::optional<fs::path> CreatePrintPatchesImage(
    const cv::Mat& debug_image,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GeneratePrintPatchesFilename(ctx);
    if (filename.empty()) { // Check if filename generation was skipped (e.g., not requested)
        return std::nullopt;
    }
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Write file (content is already generated)
    if (OutputWriter::WriteDebugImage(debug_image, full_path, log_stream)) {
        return full_path;
    }
    return std::nullopt;
}

// --- Test Chart Image ---
/**
 * @brief Creates and saves the generated test chart image. (Updated Doxygen)
 * @details Generates the chart content using internal Cairo logic and saves it via OutputWriter.
 * @param chart_opts Parameters defining the chart to generate.
 * @param ctx The context for generating the filename.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved chart file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateTestChartImage(
    const ChartGeneratorOptions& chart_opts,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateTestChartFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);

    // *** INICIO CAMBIO ***
    // 3. Generate content (Cairo surface) using the internal helper
    log_stream << _("Generating test chart content...") << std::endl; // Added log
    cairo_surface_t* surface = CreateChartSurfaceInternal(chart_opts, log_stream);
    if (!surface) {
        // Error already logged by CreateChartSurfaceInternal
        return std::nullopt;
    }

    // 4. Write file using OutputWriter
    bool success = OutputWriter::WritePng(surface, full_path, log_stream);

    // 5. Clean up surface
    cairo_surface_destroy(surface);

    if (success) {
        // OutputWriter::WritePng already logs success message like:
        // "  - Info: Plot saved to: ..."
        // We might want a more specific message here?
        log_stream << _("Test chart artifact created successfully.") << std::endl;
        return full_path;
    }
    // OutputWriter::WritePng logs failure message
    // *** FIN CAMBIO ***
    return std::nullopt;
}

// --- Corner Debug Image ---
std::optional<fs::path> CreateCornerDebugImage(
    const cv::Mat& debug_image,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateCornerDebugFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Write file (content is already generated)
    if (OutputWriter::WriteDebugImage(debug_image, full_path, log_stream)) {
        return full_path;
    }
    return std::nullopt;
}

std::optional<fs::path> CreateLogFile(
    const std::string& log_content,
    const OutputNamingContext& ctx,
    const fs::path& base_output_directory)
{
    // 1. Generate filename (base + suffix + extension)
    // Use constant from core/utils/Constants.hpp
    std::string filename_str = std::string(DynaRange::Utils::Constants::LOG_OUTPUT_FILENAME);
    // Add camera suffix if present (using the now public GetSafeCameraSuffix)
    filename_str.insert(filename_str.find_last_of('.'), OutputFilenameGenerator::GetSafeCameraSuffix(ctx));

    fs::path filename = filename_str;
    // 2. Get full path
    fs::path full_path = base_output_directory / filename;
    // 3. Write file
    std::ofstream log_file_stream(full_path);
    if (log_file_stream.is_open()) {
        log_file_stream << log_content;
        log_file_stream.close();
                
        return full_path;
    } else {
        return std::nullopt;
    }
}

/**
 * @brief Generates a small, in-memory thumbnail of a test chart.
 * (Implementation moved from ChartGenerator.cpp)
 * @param opts A struct containing all validated chart parameters.
 * @param thumb_width The desired width of the thumbnail in pixels.
 * @return An optional containing the generated thumbnail data, or nullopt on failure.
 */
std::optional<InMemoryImage> GenerateChartThumbnail(const ChartGeneratorOptions& opts, int thumb_width)
{
    // Create a temporary options struct with the thumbnail width
    ChartGeneratorOptions thumb_opts = opts;
    thumb_opts.dim_x = thumb_width; // Set desired width

    // Use the internal helper (CreateChartSurfaceInternal) to create the surface
    // Note: We use std::cerr for logging here as this function is used by the GUI
    // and may not have access to the main log_stream.
    cairo_surface_t* surface = CreateChartSurfaceInternal(thumb_opts, std::cerr);
    if (!surface) {
        // Error already logged by CreateChartSurfaceInternal
        return std::nullopt; // Return empty optional on failure
    }

    // Extract data from Cairo surface
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    unsigned char* cairo_data = cairo_image_surface_get_data(surface);
    cairo_surface_flush(surface); // Ensure data is ready

    if (!cairo_data) {
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

    // Create a buffer for generic RGB image data.
    std::vector<unsigned char> rgb_data(width * height * 3);

    // Convert Cairo's ARGB pixel data (assuming BGRA) to a simple RGB buffer.
    int cairo_stride = cairo_image_surface_get_stride(surface);
    int wx_stride = width * 3;
    for (int y = 0; y < height; ++y) {
        unsigned char* cairo_row = cairo_data + y * cairo_stride;
        unsigned char* rgb_row = rgb_data.data() + y * wx_stride;
        for (int x = 0; x < width; ++x) {
            rgb_row[x * 3 + 0] = cairo_row[x * 4 + 2]; // R
            rgb_row[x * 3 + 1] = cairo_row[x * 4 + 1]; // G
            rgb_row[x * 3 + 2] = cairo_row[x * 4 + 0]; // B
        }
    }

    // Clean up Cairo surface
    cairo_surface_destroy(surface);

    // Return the populated InMemoryImage struct
    return InMemoryImage{std::move(rgb_data), width, height};
}
} // namespace ArtifactFactory