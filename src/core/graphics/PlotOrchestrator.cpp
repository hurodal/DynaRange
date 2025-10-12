// File: src/core/graphics/PlotOrchestrator.cpp
/**
 * @file PlotOrchestrator.cpp
 * @brief Implements the central plot orchestration function.
 */
#include "PlotOrchestrator.hpp"
#include "PlotBase.hpp"
#include "PlotData.hpp"
#include "PlotDataGenerator.hpp"
#include "PlotInfoBox.hpp"
#include <algorithm>
#include <iomanip>
#include <libintl.h>
#include <sstream>

#define _(string) gettext(string)

namespace DynaRange::Graphics {

void DrawPlotToCairoContext(
    cairo_t* cr, const RenderContext& ctx, const std::vector<CurveData>& curves, const std::vector<DynamicRangeResult>& results, const std::string& title, const ProgramOptions& opts)
{
    if (curves.empty()) {
        return;
    }

    // --- Common Logic: Prepare data for plotting ---
    std::vector<CurveData> curves_with_points = curves;
    for (auto& curve : curves_with_points) {
        curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
    }

    // --- Common Logic: Calculate plot boundaries ---
    double min_ev_global = 1e6, max_ev_global = -1e6;
    double min_db_global = 1e6, max_db_global = -1e6;
    for (const auto& curve : curves_with_points) {
        if (!curve.points.empty()) {
            auto minmax_ev_it = std::minmax_element(curve.points.begin(), curve.points.end(), [](const PointData& a, const PointData& b) { return a.ev < b.ev; });
            min_ev_global = std::min(min_ev_global, minmax_ev_it.first->ev);
            max_ev_global = std::max(max_ev_global, minmax_ev_it.second->ev);

            auto minmax_db_it = std::minmax_element(curve.points.begin(), curve.points.end(), [](const PointData& a, const PointData& b) { return a.snr_db < b.snr_db; });
            min_db_global = std::min(min_db_global, minmax_db_it.first->snr_db);
            max_db_global = std::max(max_db_global, minmax_db_it.second->snr_db);
        }
    }

    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_global) - 1.0;
    bounds["max_ev"] = (max_ev_global < 0.0) ? 0.0 : ceil(max_ev_global) + 1.0;
    bounds["min_db"] = floor(min_db_global / 5.0) * 5.0;
    bounds["max_db"] = ceil(max_db_global / 5.0) * 5.0;

    // --- Common Logic: Prepare Info Box ---
    PlotInfoBox info_box;
    std::stringstream black_ss, sat_ss;
    black_ss << std::fixed << std::setprecision(2) << opts.dark_value;
    info_box.AddItem(_("Black"), black_ss.str(), opts.black_level_is_default ? _(" (estimated)") : "");
    sat_ss << std::fixed << std::setprecision(2) << opts.saturation_value;
    info_box.AddItem(_("Saturation"), sat_ss.str(), opts.saturation_level_is_default ? _(" (estimated)") : "");

    std::string command_text = curves_with_points.empty() ? "" : curves_with_points[0].generated_command;

    // --- Common Logic: Call low-level drawing functions in sequence ---
    DrawPlotBase(cr, ctx, title, opts, bounds, command_text, opts.snr_thresholds_db);
    DrawCurvesAndData(cr, ctx, info_box, curves_with_points, results, bounds, opts);
}
} // namespace DynaRange::Graphics