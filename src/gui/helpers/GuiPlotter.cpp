// File: src/gui/helpers/GuiPlotter.cpp
/**
 * @file GuiPlotter.cpp
 * @brief Implements the GUI-specific plotter.
 */
#include "GuiPlotter.hpp"
#include "../../core/graphics/PlotOrchestrator.hpp"
#include "../../core/graphics/RenderContext.hpp"
#include "../../gui/Constants.hpp" // For GUI_RENDER_SCALE_FACTOR
#include <cairo/cairo.h>
#include <libintl.h>

#define _(string) gettext(string)

namespace { // Anonymous namespace for internal helpers

/**
 * @brief Converts a Cairo image surface to a wxImage object (GUI-side utility).
 * @param surface The cairo_surface_t of type CAIRO_SURFACE_TYPE_IMAGE to convert.
 * @return A wxImage containing the graphical data.
 */
wxImage CairoSurfaceToWxImage(cairo_surface_t* surface)
{
    if (!surface || cairo_surface_get_type(surface) != CAIRO_SURFACE_TYPE_IMAGE) {
        return wxImage(); // Return invalid image
    }

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    unsigned char* cairo_data = cairo_image_surface_get_data(surface);
    cairo_surface_flush(surface); // Ensure all drawing is complete

    if (!cairo_data) {
        return wxImage();
    }

    wxImage image(width, height);
    unsigned char* rgb_data = image.GetData();

    // Convert Cairo's BGRA format to wxImage's RGB format.
    for (int i = 0; i < width * height; ++i) {
        rgb_data[i * 3 + 0] = cairo_data[i * 4 + 2]; // R
        rgb_data[i * 3 + 1] = cairo_data[i * 4 + 1]; // G
        rgb_data[i * 3 + 2] = cairo_data[i * 4 + 0]; // B
    }

    return image;
}

} // end anonymous namespace

namespace GuiPlotter {

wxImage GeneratePlotAsWxImage(
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const std::string& title,
    const ReportingParameters& reporting_params)
{
    if (curves.empty()) {
        return wxImage();
    }

    // 1. Calculate GUI canvas dimensions and create the context.
    const int gui_width = static_cast<int>(DynaRange::Graphics::Constants::PlotDefs::BASE_WIDTH * DynaRange::Gui::Constants::GUI_RENDER_SCALE_FACTOR);
    const int gui_height = static_cast<int>(DynaRange::Graphics::Constants::PlotDefs::BASE_HEIGHT * DynaRange::Gui::Constants::GUI_RENDER_SCALE_FACTOR);
    const auto render_ctx = DynaRange::Graphics::RenderContext{gui_width, gui_height};
    
    // 2. Prepare the in-memory Cairo surface.
    cairo_surface_t* surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, render_ctx.base_width, render_ctx.base_height);
    cairo_t* cr = cairo_create(surface);

    // 3. Call the central "skeleton" function to do all the drawing.
    DynaRange::Graphics::DrawPlotToCairoContext(cr, render_ctx, curves, results, title, reporting_params);
    
    // 4. Convert the result to a wxImage.
    wxImage final_image = CairoSurfaceToWxImage(surface);
    
    // 5. Clean up.
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return final_image;
}

} // namespace GuiPlotter