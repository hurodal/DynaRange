// File: src/gui/helpers/GuiPlotter.cpp
/**
 * @file GuiPlotter.cpp
 * @brief Implements the GUI-specific plotter.
 */
#include "GuiPlotter.hpp"
#include "../../core/graphics/PlotBoundsCalculator.hpp"
#include "../../core/graphics/PlotDataGenerator.hpp"
#include "../../core/graphics/PlotOrchestrator.hpp"
#include "../../core/graphics/RenderContext.hpp"
#include "../../core/utils/PlotTitleGenerator.hpp"
#include "../../gui/Constants.hpp"
#include <cairo/cairo.h>
#include <libintl.h>
#include <set>

namespace { // Anonymous namespace for internal helpers

/**
 * @brief Converts a Cairo image surface to a wxImage object (GUI-side utility).
 * @param surface The cairo_surface_t of type CAIRO_SURFACE_TYPE_IMAGE to convert.
 * @return A wxImage containing the graphical data. Returns invalid image on failure.
 */
wxImage CairoSurfaceToWxImage(cairo_surface_t* surface)
{
    // Check if surface is valid and is an image surface
    if (!surface || cairo_surface_get_type(surface) != CAIRO_SURFACE_TYPE_IMAGE) {
        return wxImage(); // Return invalid image if not an image surface
    }

    // Get dimensions and data pointer from Cairo surface
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    unsigned char* cairo_data = cairo_image_surface_get_data(surface);
    cairo_surface_flush(surface); // Ensure all drawing operations are completed

    // Check if data pointer is valid
    if (!cairo_data) {
        return wxImage(); // Return invalid image if data is null
    }

    // Create a wxImage of the correct size
    // wxImage uses RGB format internally
    wxImage image(width, height);
    // Get pointer to wxImage's internal buffer (must call GetData() AFTER setting size)
    unsigned char* rgb_data = image.GetData();

    // Check if wxImage buffer allocation succeeded
    if (!rgb_data) {
        return wxImage(); // Return invalid image if GetData failed
    }

    // Convert Cairo's ARGB32 (often BGRA on little-endian) format to wxImage's RGB format.
    // Assuming Cairo uses BGRA byte order here. Adjust if necessary for your platform.
    int cairo_stride = cairo_image_surface_get_stride(surface); // Bytes per row in Cairo data
    int wx_stride = width * 3; // Bytes per row in wxImage data (RGB)

    for (int y = 0; y < height; ++y) {
        unsigned char* cairo_row = cairo_data + y * cairo_stride;
        unsigned char* wx_row = rgb_data + y * wx_stride;
        for (int x = 0; x < width; ++x) {
            // Assuming BGRA order for Cairo ARGB32 on little-endian
            wx_row[x * 3 + 0] = cairo_row[x * 4 + 2]; // R = Cairo BGR[A]'s R
            wx_row[x * 3 + 1] = cairo_row[x * 4 + 1]; // G = Cairo BGR[A]'s G
            wx_row[x * 3 + 2] = cairo_row[x * 4 + 0]; // B = Cairo BGR[A]'s B
            // Alpha channel (cairo_row[x * 4 + 3]) is ignored by wxImage GetData buffer
        }
    }

    return image;
}
} // end anonymous namespace

namespace GuiPlotter {

// Function modified to use PlotTitleGenerator
wxImage GeneratePlotAsWxImage(const std::vector<CurveData>& curves, const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx, // Parameter changed
    const ReportingParameters& reporting_params)
{
    if (curves.empty()) {
        return wxImage(); // Return invalid image
    }

    // 1. Calculate GUI canvas dimensions and create the render context.
    const int gui_width = static_cast<int>(DynaRange::Graphics::Constants::PlotDefs::BASE_WIDTH * DynaRange::Gui::Constants::GUI_RENDER_SCALE_FACTOR);
    const int gui_height = static_cast<int>(DynaRange::Graphics::Constants::PlotDefs::BASE_HEIGHT * DynaRange::Gui::Constants::GUI_RENDER_SCALE_FACTOR);
    const auto render_ctx = DynaRange::Graphics::RenderContext { gui_width, gui_height };

    // 2. Prepare data for plotting: generate curve points and calculate global bounds
    std::vector<CurveData> curves_with_points = curves;
    for (auto& curve : curves_with_points) {
        curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
    }
    const auto bounds = DynaRange::Graphics::CalculateGlobalBounds(curves_with_points);

    // Generate title using PlotTitleGenerator
    std::string plot_title;
    // Determine if it's a summary or individual plot based on unique filenames
    std::set<std::string> unique_filenames;
    for (const auto& curve : curves) {
        unique_filenames.insert(curve.filename);
    }
    if (unique_filenames.size() > 1 || curves.size() == 0) { // Treat empty case as summary for safety
        plot_title = PlotTitleGenerator::GenerateSummaryTitle(ctx);
    } else {
        // Assume single file -> Individual plot. Context must have ISO populated by caller.
        plot_title = PlotTitleGenerator::GenerateIndividualTitle(ctx);
        if (plot_title.empty() && !curves.empty()) {
            // Fallback if individual title generation failed (e.g., missing ISO in context)
            plot_title = curves[0].filename; // Use filename as fallback
        }
    }

    // 3. Prepare the in-memory Cairo surface.
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, render_ctx.base_width, render_ctx.base_height);
    cairo_t* cr = cairo_create(surface);

    // 4. Call the central drawing function with the generated title
    DynaRange::Graphics::DrawPlotToCairoContext(cr, render_ctx, curves_with_points, results, plot_title, reporting_params, bounds);

    // 5. Convert the result to a wxImage.
    wxImage final_image = CairoSurfaceToWxImage(surface);

    // 6. Clean up.
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return final_image;
}

} // namespace GuiPlotter