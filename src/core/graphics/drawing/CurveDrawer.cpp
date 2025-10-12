// File: src/core/graphics/drawing/CurveDrawer.cpp
/**
 * @file CurveDrawer.cpp
 * @brief Implements the drawing logic for plot curves and scatter points.
 */
#include "CurveDrawer.hpp"
#include "../PlotBase.hpp" // For MapToPixelCoords
#include "../Colour.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace DynaRange::Graphics::Drawing {

void CurveDrawer::Draw(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const RenderContext& ctx, double alpha) const
{
    if (curve.curve_points.empty()) {
        return;
    }
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };
    
    PlotColors::SetSourceFromChannelWithAlpha(cr, curve.channel, alpha);
    cairo_set_line_width(cr, 2.0);

    const auto& first_point = curve.curve_points[0];
    auto [start_x, start_y] = map_coords(first_point.first, first_point.second);
    cairo_move_to(cr, start_x, start_y);

    for (size_t i = 1; i < curve.curve_points.size(); ++i) {
        const auto& point = curve.curve_points[i];
        auto [x, y] = map_coords(point.first, point.second);
        cairo_line_to(cr, x, y);
    }
    cairo_stroke(cr);
}

void CurveDrawer::DrawPoints(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const RenderContext& ctx, double alpha) const
{
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };

    if (curve.channel == DataSource::AVG) {
        for (const auto& point : curve.points) {
            PlotColors::SetSourceFromChannelWithAlpha(cr, point.channel, alpha);
            auto [px, py] = map_coords(point.ev, point.snr_db);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    } else {
        PlotColors::SetSourceFromChannelWithAlpha(cr, curve.channel, alpha);
        for (const auto& point : curve.points) {
            auto [px, py] = map_coords(point.ev, point.snr_db);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
}

} // namespace DynaRange::Graphics::Drawing