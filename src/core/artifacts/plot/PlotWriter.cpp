// File: src/core/artifacts/plot/PlotWriter.cpp
/**
 * @file src/core/artifacts/plot/PlotWriter.cpp
 * @brief Implements functions for generating and saving plot image artifacts.
 */
#include "PlotWriter.hpp"
#include "../../graphics/PlotBoundsCalculator.hpp"
#include "../../graphics/PlotDataGenerator.hpp"
#include "../../graphics/PlotOrchestrator.hpp"
#include "../../graphics/RenderContext.hpp"
#include "../../graphics/Constants.hpp"
#include "../../utils/OutputFilenameGenerator.hpp"
#include "../../utils/PlotTitleGenerator.hpp"
#include "../../io/OutputWriter.hpp"
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>
#include <iostream>
#include <libintl.h>

#define _(string) gettext(string)

namespace { // Anonymous namespace for internal helpers

/**
 * @brief Internal helper to generate and save a plot artifact (PNG, PDF, or SVG).
 * This function encapsulates the core plotting and saving logic previously in ArtifactFactory.cpp.
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

namespace ArtifactFactory::Plot {

// --- Summary Plot ---
std::optional<fs::path> CreateSummaryPlot(
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream)
{
    if (!reporting_params.generate_plot || curves.empty()) {
        if (curves.empty() && reporting_params.generate_plot) {
           log_stream << _("  - Warning: Skipping summary plot generation due to no curve data.") << std::endl;
        }
        return std::nullopt;
    }

    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateSummaryPlotFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Generate title
    std::string title = PlotTitleGenerator::GenerateSummaryTitle(ctx);

    // 4. Prepare curve points and calculate bounds
    std::vector<CurveData> curves_with_points = curves; 
    for (auto& curve : curves_with_points) {
        if (curve.curve_points.empty()) {
             curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
        }
    }
    const auto bounds = DynaRange::Graphics::CalculateGlobalBounds(curves_with_points);

    // 5. Call the internal plotting helper
    std::optional<std::string> result_path_str = GeneratePlotInternal(
        full_path.string(), title, curves_with_points, results, reporting_params, bounds, log_stream);

    if (result_path_str) {
        return fs::path(*result_path_str);
    }
    return std::nullopt;
}

// --- Individual Plot ---
std::optional<fs::path> CreateIndividualPlot(
    const std::vector<CurveData>& curves_for_file,
    const std::vector<DynamicRangeResult>& results_for_file,
    const OutputNamingContext& ctx, 
    const ReportingParameters& reporting_params,
    const std::map<std::string, double>& global_bounds, 
    const PathManager& paths,
    std::ostream& log_stream)
{
    if (!reporting_params.generate_plot || !reporting_params.generate_individual_plots || curves_for_file.empty()) {
        return std::nullopt; 
    }

    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateIndividualPlotFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Generate title
    std::string title = PlotTitleGenerator::GenerateIndividualTitle(ctx);
    
    if (title.empty()) {
        title = fs::path(curves_for_file[0].filename).filename().string();
    }

    // 4. Prepare curve points (ensure they exist)
    std::vector<CurveData> curves_with_points = curves_for_file; 
    for (auto& curve : curves_with_points) {
        if (curve.curve_points.empty()) {
            curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
        }
    }

    // 5. Call the internal plotting helper
    std::optional<std::string> result_path_str = GeneratePlotInternal(
        full_path.string(), title, curves_with_points, results_for_file, reporting_params, global_bounds, log_stream);

    if (result_path_str) {
        return fs::path(*result_path_str);
    }
    return std::nullopt;
}

} // namespace ArtifactFactory::Plot