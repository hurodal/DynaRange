// File: src/core/graphics/drawing/GridDrawer.cpp
/**
 * @file GridDrawer.cpp
 * @brief Implements the drawing logic for plot grid and threshold lines.
 */
#include "GridDrawer.hpp"
#include "../FontManager.hpp"
#include "../PlotBase.hpp"
#include "../Colour.hpp"
#include <libintl.h>
#include <sstream>
#include <iomanip>

#define _(string) gettext(string)

namespace DynaRange::Graphics::Drawing {

void GridDrawer::Draw(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx, const std::vector<double>& snr_thresholds) const
{
    DrawGridLines(cr, bounds, ctx);
    DrawThresholdLines(cr, bounds, snr_thresholds, ctx);
}

void GridDrawer::DrawGridLines(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const
{
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };

    PlotColors::cairo_set_source_grey_90(cr);
    cairo_set_line_width(cr, 1.0);

    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        auto [p1x, p1y] = map_coords(ev, bounds.at("min_db"));
        auto [p2x, p2y] = map_coords(ev, bounds.at("max_db"));
        cairo_move_to(cr, p1x, p1y);
        cairo_line_to(cr, p2x, p2y);
        cairo_stroke(cr);
    }

    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), db);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), db);
        cairo_move_to(cr, p1x, p1y);
        cairo_line_to(cr, p2x, p2y);
        cairo_stroke(cr);
    }
}

void GridDrawer::DrawThresholdLines(cairo_t* cr, const std::map<std::string, double>& bounds, const std::vector<double>& snr_thresholds, const RenderContext& ctx) const
{
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };
    const FontManager font_manager(ctx);

    PlotColors::cairo_set_source_grey_20(cr);
    cairo_set_line_width(cr, 2.0);
    font_manager.SetThresholdLabelFont(cr);

    for (const double threshold : snr_thresholds) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), threshold);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), threshold);
        DrawDashedLine(cr, p1x, p1y, p2x, p2y, 20.0);
        
        std::stringstream ss;
        ss << "SNR > " << std::fixed << std::setprecision(1) << threshold << "dB";
        cairo_move_to(cr, p1x + 20, p1y - 10);
        cairo_show_text(cr, ss.str().c_str());
    }
}

void GridDrawer::DrawDashedLine(cairo_t* cr, double x1, double y1, double x2, double y2, double dash_length) const
{
    double dashes[] = { dash_length, dash_length };
    cairo_save(cr);
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
    cairo_restore(cr);
}

} // namespace DynaRange::Graphics::Drawing