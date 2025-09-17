// File: src/core/graphics/PlotBase.hpp
/**
 * @file src/core/graphics/PlotBase.hpp
 * @brief Declares the function to draw the static base of a plot (axes, grid, titles, etc.).
 */
#pragma once
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>

/// @brief The width of the generated plot images in pixels.
constexpr int PLOT_WIDTH = 1920;
/// @brief The height of the generated plot images in pixels.
constexpr int PLOT_HEIGHT = 1080;

// Margins (defined here so they can be used by both PlotBase and PlotData)
constexpr int MARGIN_LEFT = 180;
constexpr int MARGIN_RIGHT = 100;
constexpr int MARGIN_TOP = 100;
constexpr int MARGIN_BOTTOM = 120;

/**
 * @brief Draws the static base of a plot (axes, grid, titles, threshold lines).
 * @param cr The cairo drawing context.
 * @param title The main title of the plot.
 * @param bounds A map containing the plot boundaries (min_ev, max_ev, min_db, max_db).
 * @param command_text The command-line text to display at the bottom of the plot.
 * @param snr_thresholds A vector of SNR thresholds to draw as horizontal dashed lines.
 */
void DrawPlotBase(
    cairo_t* cr,
    const std::string& title,
    const std::map<std::string, double>& bounds,
    const std::string& command_text,
    const std::vector<double>& snr_thresholds);