// File: src/gui/helpers/GuiPlotter.cpp
/**
 * @file GuiPlotter.cpp
 * @brief Implements the GUI-specific plotter.
 */
#include "GuiPlotter.hpp"
#include "../../core/graphics/PlotBase.hpp"
#include "../../core/graphics/PlotData.hpp"
#include "../../core/graphics/PlotInfoBox.hpp"
#include "../../core/graphics/PlotDataGenerator.hpp"
#include "../../core/graphics/RenderContext.hpp"
#include "../../gui/Constants.hpp" // For GUI_RENDER_SCALE_FACTOR
#include <cairo/cairo.h>
#include <algorithm>
#include <libintl.h>
#include <iomanip>

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

// NOTE: This is an existing function that has been modified.
wxImage GeneratePlotAsWxImage(
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const std::string& title,
    const ProgramOptions& opts)
{
    if (curves.empty()) {
        return wxImage();
    }

    // Calculate GUI canvas dimensions using the base resolution and the GUI scale factor
    const int gui_width = static_cast<int>(DynaRange::Graphics::Constants::PlotDefs::BASE_WIDTH * DynaRange::Gui::Constants::GUI_RENDER_SCALE_FACTOR);
    const int gui_height = static_cast<int>(DynaRange::Graphics::Constants::PlotDefs::BASE_HEIGHT * DynaRange::Gui::Constants::GUI_RENDER_SCALE_FACTOR);
    const auto render_ctx = DynaRange::Graphics::RenderContext{gui_width, gui_height};

    std::vector<CurveData> curves_with_points = curves;
    for (auto& curve : curves_with_points) {
        curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
    }
    
    // Calculate plot boundaries
    double min_ev_global = 1e6, max_ev_global = -1e6;
    double min_db_global = 1e6, max_db_global = -1e6;
    for (const auto& curve : curves_with_points) {
        if (!curve.points.empty()) {
            auto minmax_ev_it = std::minmax_element(curve.points.begin(), curve.points.end(),
                [](const PointData& a, const PointData& b){ return a.ev < b.ev; });
            min_ev_global = std::min(min_ev_global, minmax_ev_it.first->ev);
            max_ev_global = std::max(max_ev_global, minmax_ev_it.second->ev);

            auto minmax_db_it = std::minmax_element(curve.points.begin(), curve.points.end(),
                [](const PointData& a, const PointData& b){ return a.snr_db < b.snr_db; });
            min_db_global = std::min(min_db_global, minmax_db_it.first->snr_db);
            max_db_global = std::max(max_db_global, minmax_db_it.second->snr_db);
        }
    }

    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_global) - 1.0;
    bounds["max_ev"] = (max_ev_global < 0.0) ? 0.0 : ceil(max_ev_global) + 1.0;
    bounds["min_db"] = floor(min_db_global / 5.0) * 5.0;
    bounds["max_db"] = ceil(max_db_global / 5.0) * 5.0;

    // Render to an in-memory Cairo surface
    cairo_surface_t* surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, render_ctx.base_width, render_ctx.base_height);
    cairo_t* cr = cairo_create(surface);

    PlotInfoBox info_box;
    std::stringstream black_ss, sat_ss;
    black_ss << std::fixed << std::setprecision(2) << opts.dark_value;
    info_box.AddItem(_("Black"), black_ss.str(), opts.black_level_is_default ? _(" (estimated)") : "");
    sat_ss << std::fixed << std::setprecision(2) << opts.saturation_value;
    info_box.AddItem(_("Saturation"), sat_ss.str(), opts.saturation_level_is_default ? _(" (estimated)") : "");

    // Use the flexible drawing functions with the GUI context
    DrawPlotBase(cr, render_ctx, title, opts, bounds, curves[0].generated_command, opts.snr_thresholds_db);
    DrawCurvesAndData(cr, render_ctx, info_box, curves_with_points, results, bounds);
    DrawGeneratedTimestamp(cr, render_ctx);
    
    wxImage final_image = CairoSurfaceToWxImage(surface);

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return final_image;
}

} // namespace GuiPlotter