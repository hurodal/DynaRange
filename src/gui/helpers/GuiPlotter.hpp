// File: src/gui/helpers/GuiPlotter.hpp
/**
 * @file GuiPlotter.hpp
 * @brief Declares a GUI-specific plotter to render core data into a wxImage.
 */
#pragma once

#include "../../core/analysis/Analysis.hpp"
#include "../../core/engine/Reporting.hpp"
#include "../../core/utils/OutputNamingContext.hpp"
#include <wx/image.h>
#include <vector>

namespace GuiPlotter {
    /**
     * @brief Renders a plot from core data structures into a wxImage for GUI display.
     * @param curves The vector of CurveData containing the points and coefficients to plot.
     * @param results The vector of DynamicRangeResult for plotting intersection labels.
     * @param ctx The OutputNamingContext with data needed for title generation and plotting options. // Parameter changed
     * @param reporting_params The parameters required for rendering the plot details (distinct from naming context).
     * @return A wxImage containing the rendered plot, ready for display. Returns invalid image on error or if curves are empty.
     */
    wxImage GeneratePlotAsWxImage(
        const std::vector<CurveData>& curves,
        const std::vector<DynamicRangeResult>& results,
        const OutputNamingContext& ctx,
        const ReportingParameters& reporting_params
    );
}