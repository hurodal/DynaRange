// File: src/core/graphics/PlotData.cpp
/**
 * @file src/core/graphics/PlotData.cpp
 * @brief Implements the low-level Cairo drawing functions for the dynamic plot data.
 */
#include "PlotData.hpp"
#include "Colour.hpp"
#include "drawing/CurveDrawer.hpp" 
#include "drawing/LabelDrawer.hpp" 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void DrawCurvesAndData(
    cairo_t* cr,
    const DynaRange::Graphics::RenderContext& ctx,
    const PlotInfoBox& info_box,
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const std::map<std::string, double>& bounds,
    const ProgramOptions& opts)
{
    info_box.Draw(cr, ctx);

    // --- Define the desired drawing order (from back to front) ---
    const std::vector<DataSource> draw_order = { DataSource::AVG, DataSource::G1, DataSource::G2, DataSource::R, DataSource::B };
    
    std::vector<const CurveData*> sorted_curves;
    for (DataSource channel_to_find : draw_order) {
        for (const auto& curve : curves) {
            if (curve.channel == channel_to_find) {
                sorted_curves.push_back(&curve);
            }
        }
    }

    // --- PASS 1: Draw geometric shapes using CurveDrawer ---
    const DynaRange::Graphics::Drawing::CurveDrawer curve_drawer;
    for (size_t i = 0; i < sorted_curves.size(); ++i) {
        const CurveData& curve = *sorted_curves[i];
        if (curve.points.empty())
            continue;
        
        double alpha = (i == 0) ? 1.0 : (1.0 - PlotColors::OPACITY_DECREMENT_STEP);
        
        if (opts.plot_details.show_curve) {
            curve_drawer.Draw(cr, curve, bounds, ctx, alpha);
        }
        if (opts.plot_details.show_scatters) {
            curve_drawer.DrawPoints(cr, curve, bounds, ctx, alpha);
        }
    }

    // --- PASS 2: Draw textual labels using LabelDrawer ---
    if (!opts.plot_details.show_labels) {
        return; // Skip all label drawing if disabled
    }

    const DynaRange::Graphics::Drawing::LabelDrawer label_drawer;
    label_drawer.Draw(cr, curves, results, bounds, ctx);
}