// File: src/core/graphics/PlotOrchestrator.hpp
/**
 * @file PlotOrchestrator.hpp
 * @brief Declares the central plot orchestration function.
 * @details This module contains the "skeleton" function that handles all
 * common plot drawing logic, making it reusable for different output targets.
 */
#pragma once

#include "RenderContext.hpp"
#include "../analysis/Analysis.hpp"
#include "../arguments/ArgumentsOptions.hpp"
#include <cairo/cairo.h>
#include <vector>
#include <string>

namespace DynaRange::Graphics {

    /**
     * @brief Draws a complete plot onto a provided Cairo context.
     * @details This is the central "skeleton" function for all plotting. It
     * calculates boundaries, prepares data, and calls the low-level drawing
     * functions in the correct sequence. It is agnostic of the final output format.
     * @param cr The Cairo context to draw on.
     * @param ctx The rendering context defining the canvas dimensions.
     * @param curves The curve data to be plotted.
     * @param results The dynamic range results for labeling.
     * @param title The main title for the plot.
     * @param opts The program options used for the analysis.
     */
    void DrawPlotToCairoContext(
        cairo_t* cr,
        const RenderContext& ctx,
        const std::vector<CurveData>& curves,
        const std::vector<DynamicRangeResult>& results,
        const std::string& title,
        const ProgramOptions& opts
    );

} // namespace DynaRange::Graphics