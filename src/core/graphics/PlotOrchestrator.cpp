// File: src/core/graphics/PlotOrchestrator.cpp
/**
 * @file PlotOrchestrator.cpp
 * @brief Implements the central plot orchestration function.
 */
#include "PlotOrchestrator.hpp"
#include "PlotBase.hpp"
#include "PlotData.hpp"
#include "PlotInfoBox.hpp"
#include <iomanip>
#include <libintl.h>
#include <sstream>

#define _(string) gettext(string)

namespace DynaRange::Graphics {

void DrawPlotToCairoContext(
    cairo_t* cr, const RenderContext& ctx, const std::vector<CurveData>& curves, const std::vector<DynamicRangeResult>& results, const std::string& title, const ReportingParameters& reporting_params, const std::map<std::string, double>& bounds)
{
    if (curves.empty()) {
        return;
    }

    // --- Common Logic: Prepare Info Box ---
    PlotInfoBox info_box;
    std::stringstream black_ss, sat_ss;
    black_ss << std::fixed << std::setprecision(2) << reporting_params.dark_value;
    info_box.AddItem(_("Black"), black_ss.str(), reporting_params.black_level_is_default ? _(" (estimated)") : "");
    sat_ss << std::fixed << std::setprecision(2) << reporting_params.saturation_value;
    info_box.AddItem(_("Saturation"), sat_ss.str(), reporting_params.saturation_level_is_default ? _(" (estimated)") : "");

    // --- Common Logic: Call low-level drawing functions in sequence ---
    // The bounds are now passed directly instead of being calculated here.
    DrawPlotBase(cr, ctx, title, reporting_params.raw_channels, bounds, reporting_params.generated_command, reporting_params.snr_thresholds_db);
    DrawCurvesAndData(cr, ctx, info_box, curves, results, bounds, reporting_params.plot_details);
}
} // namespace DynaRange::Graphics