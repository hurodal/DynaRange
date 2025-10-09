// File: src/gui/helpers/GuiPlotter.hpp
/**
 * @file GuiPlotter.hpp
 * @brief Declares a GUI-specific plotter to render core data into a wxImage.
 */
#pragma once

#include "../../core/analysis/Analysis.hpp"
#include "../../core/arguments/ArgumentsOptions.hpp"
#include <wx/image.h>
#include <vector>
#include <string>

namespace GuiPlotter {
    /**
     * @brief Renders a plot from core data structures into a wxImage for GUI display.
     * @param curves The vector of CurveData containing the points and coefficients to plot.
     * @param results The vector of DynamicRangeResult for plotting intersection labels.
     * @param title The main title of the plot.
     * @param opts The program options used for the analysis.
     * @return A wxImage containing the rendered plot, ready for display.
     */
    wxImage GeneratePlotAsWxImage(
        const std::vector<CurveData>& curves,
        const std::vector<DynamicRangeResult>& results,
        const std::string& title,
        const ProgramOptions& opts
    );
}