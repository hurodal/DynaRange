// File: src/core/graphics/PlotData.hpp
/**
 * @file src/core/graphics/PlotData.hpp
 * @brief Declares the function to draw the dynamic data onto a plot (curves, points, labels).
 */
#pragma once
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>
#include "../analysis/Analysis.hpp"
#include "PlotInfoBox.hpp"
#include "RenderContext.hpp"
#include "../arguments/ArgumentsOptions.hpp"


/**
 * @brief Draws the dynamic data onto the plot (data points, curves, labels).
 * @param cr The cairo drawing context.
 * @param ctx The rendering context with canvas dimensions.
 * @param info_box The info box containing plot titles.
 * @param curves A vector of CurveData structs, each representing a curve to draw.
 * @param results A vector of DynamicRangeResult, containing DR data for labels.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 * @param plot_details The parameters that control which plot elements to draw.
 */
void DrawCurvesAndData(
    cairo_t* cr,
    const DynaRange::Graphics::RenderContext& ctx,
    const PlotInfoBox& info_box,
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const std::map<std::string, double>& bounds,
    const PlottingDetails& plot_details);