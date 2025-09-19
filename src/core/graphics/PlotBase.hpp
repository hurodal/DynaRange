// File: src/core/graphics/PlotBase.hpp
/**
 * @file src/core/graphics/PlotBase.hpp
 * @brief Declares the function to draw the static base of a plot (axes, grid, titles, etc.).
 */
#pragma once
#include "../arguments/ProgramOptions.hpp"
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
 * @brief Maps data coordinates (EV, dB) to pixel coordinates on the plot.
 * @param ev Exposure value (x-axis).
 * @param db SNR in dB (y-axis).
 * @param bounds Map containing plot boundaries (min_ev, max_ev, min_db, max_db).
 * @return Pair of pixel coordinates (x, y).
 * @details This function is defined as 'inline' to allow safe inclusion from multiple .cpp files.
 */
inline std::pair<double, double> MapToPixelCoords(double ev, double db, const std::map<std::string, double>& bounds) {
    const int plot_area_width = PLOT_WIDTH - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = PLOT_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;
    double px = MARGIN_LEFT + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
    double py = (PLOT_HEIGHT - MARGIN_BOTTOM) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
    return std::make_pair(px, py);
}

/**
 * @brief Draws the static base of a plot (axes, grid, titles, threshold lines).
 * @param cr The cairo drawing context.
 * @param title The main title of the plot.
 * @param opts The program options, used to display calibration values. // <-- AÑADIDO
 * @param bounds A map containing the plot boundaries (min_ev, max_ev, min_db, max_db).
 * @param command_text The command-line text to display at the bottom of the plot.
 * @param snr_thresholds A vector of SNR thresholds to draw as horizontal dashed lines.
 */
void DrawPlotBase(
    cairo_t* cr,
    const std::string& title,
    const ProgramOptions& opts,
    const std::map<std::string, double>& bounds,
    const std::string& command_text,
    const std::vector<double>& snr_thresholds);