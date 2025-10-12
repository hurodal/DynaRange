// File: src/core/graphics/drawing/AxisDrawer.cpp
/**
 * @file AxisDrawer.cpp
 * @brief Implements the drawing logic for plot axes.
 */
#include "AxisDrawer.hpp"
#include "../FontManager.hpp"
#include "../PlotBase.hpp"
#include "../Colour.hpp"
#include <string>

namespace DynaRange::Graphics::Drawing {

void AxisDrawer::Draw(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const
{
    DrawPlotBackgroundAndBorder(cr, ctx);
    DrawXAxisLabels(cr, bounds, ctx);
    DrawYAxisLabels(cr, bounds, ctx);
}

void AxisDrawer::DrawPlotBackgroundAndBorder(cairo_t* cr, const RenderContext& ctx) const
{
    const int plot_area_width = ctx.base_width - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = ctx.base_height - MARGIN_TOP - MARGIN_BOTTOM;

    PlotColors::cairo_set_source_white(cr);
    cairo_rectangle(cr, 0, 0, ctx.base_width, ctx.base_height);
    cairo_fill(cr);

    PlotColors::cairo_set_source_black(cr);
    cairo_set_line_width(cr, 3.0);
    cairo_rectangle(cr, MARGIN_LEFT, MARGIN_TOP, plot_area_width, plot_area_height);
    cairo_stroke(cr);
}

void AxisDrawer::DrawXAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const
{
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };
    const FontManager font_manager(ctx);
    font_manager.SetAxisTickFont(cr);

    PlotColors::cairo_set_source_black(cr);
    cairo_text_extents_t extents;
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        std::string ev_str = std::to_string(static_cast<int>(ev));
        cairo_text_extents(cr, ev_str.c_str(), &extents);
        auto [px, py] = map_coords(ev, bounds.at("min_db"));
        cairo_move_to(cr, px - extents.width / 2, ctx.base_height - MARGIN_BOTTOM + 25);
        cairo_show_text(cr, ev_str.c_str());
    }
}

void AxisDrawer::DrawYAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const
{
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };
    const FontManager font_manager(ctx);
    font_manager.SetAxisTickFont(cr);

    PlotColors::cairo_set_source_black(cr);
    cairo_text_extents_t extents;
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        std::string db_str = std::to_string(static_cast<int>(db));
        cairo_text_extents(cr, db_str.c_str(), &extents);
        auto [px, py] = map_coords(bounds.at("min_ev"), db);
        cairo_move_to(cr, MARGIN_LEFT - extents.width - 15, py + extents.height / 2);
        cairo_show_text(cr, db_str.c_str());
    }
}

} // namespace DynaRange::Graphics::Drawing