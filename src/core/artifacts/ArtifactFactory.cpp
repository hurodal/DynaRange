// File: src/core/artifacts/ArtifactFactory.cpp
/**
 * @file src/core/artifacts/ArtifactFactory.cpp
 * @brief Implements the ArtifactFactory for creating and saving output artifacts.
 */
#include "ArtifactFactory.hpp"
#include "../utils/OutputFilenameGenerator.hpp"
#include "../io/OutputWriter.hpp"
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <libintl.h>
#include <cstring>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace { // Anonymous namespace for internal helpers

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

} // end anonymous namespace


namespace ArtifactFactory {

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