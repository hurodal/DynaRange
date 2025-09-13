/**
 * @file core/graphics/Drawing.hpp
 * @brief Declares low-level Cairo drawing functions for creating plots.
 */
#pragma once

#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>
#include "../Analysis.hpp"

/// @brief The width of the generated plot images in pixels.
constexpr int PLOT_WIDTH = 1920;
/// @brief The height of the generated plot images in pixels.
constexpr int PLOT_HEIGHT = 1080;

/**
 * @brief Draws the static base of a plot (axes, grid, titles, threshold lines).
 * @param cr The cairo drawing context.
 * @param title The main title of the plot.
 * @param bounds A map containing the plot boundaries (min_ev, max_ev, min_db, max_db).
 * @param command_text The command-line text to display at the bottom of the plot.
 * @param snr_thresholds A vector of SNR thresholds to draw as horizontal dashed lines.
 */
void DrawPlotBase(cairo_t* cr, const std::string& title, const std::map<std::string, double>& bounds, const std::string& command_text, const std::vector<double>& snr_thresholds);

/**
 * @brief Draws the dynamic data onto the plot (data points, curves, labels).
 * @param cr The cairo drawing context.
 * @param curves A vector of CurveData structs, each representing a curve to draw.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 */
void DrawCurvesAndData(cairo_t* cr, const std::vector<CurveData>& curves, const std::map<std::string, double>& bounds);