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

/**
 * @brief Draws the dynamic data onto the plot (data points, curves, labels).
 * @param cr The cairo drawing context.
 * @param curves A vector of CurveData structs, each representing a curve to draw.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 */
void DrawCurvesAndData(
    cairo_t* cr,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds);