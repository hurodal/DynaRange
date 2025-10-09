// File: src/core/graphics/PlotBase.hpp
/**
 * @file src/core/graphics/PlotBase.hpp
 * @brief Declares the function to draw the static base of a plot (axes, grid, titles, etc.).
 */
#pragma once
#include "../arguments/ArgumentsOptions.hpp"
#include "RenderContext.hpp" // Include the new RenderContext
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>


// Margins are now based on the unscaled base dimensions.
constexpr int MARGIN_LEFT = 180;
constexpr int MARGIN_RIGHT = 100;
constexpr int MARGIN_TOP = 100;
constexpr int MARGIN_BOTTOM = 120;

/**
 * @brief Maps data coordinates (EV, dB) to pixel coordinates on the plot.
 * @param ev Exposure value (x-axis).
 * @param db SNR in dB (y-axis).
 * @param bounds Map containing plot boundaries (min_ev, max_ev, min_db, max_db).
 * @param ctx The rendering context with canvas dimensions.
 * @return Pair of pixel coordinates (x, y).
 * @details This function is defined as 'inline' to allow safe inclusion from multiple .cpp files.
 */
inline std::pair<double, double> MapToPixelCoords(double ev, double db, const std::map<std::string, double>& bounds, const DynaRange::Graphics::RenderContext& ctx) {
    const int plot_area_width = ctx.base_width - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = ctx.base_height - MARGIN_TOP - MARGIN_BOTTOM;

    double px = MARGIN_LEFT + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
    double py = (ctx.base_height - MARGIN_BOTTOM) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
    return std::make_pair(px, py);
}

/**
 * @brief Draws the static base of a plot (axes, grid, titles, threshold lines).
 * @param cr The cairo drawing context.
 * @param ctx The rendering context with canvas dimensions.
 * @param title The main title of the plot.
 * @param opts The program options, used to display calibration values.
 * @param bounds A map containing the plot boundaries (min_ev, max_ev, min_db, max_db).
 * @param command_text The command-line text to display at the bottom of the plot.
 * @param snr_thresholds A vector of SNR thresholds to draw as horizontal dashed lines.
 */
void DrawPlotBase(
    cairo_t* cr,
    const DynaRange::Graphics::RenderContext& ctx,
    const std::string& title,
    const ProgramOptions& opts,
    const std::map<std::string, double>& bounds,
    const std::string& command_text,
    const std::vector<double>& snr_thresholds);
/**
 * @brief Draws a timestamp at the bottom-left of the plot.
 * @param cr The cairo drawing context.
 * @param ctx The rendering context with canvas dimensions.
 */
void DrawGeneratedTimestamp(cairo_t* cr, const DynaRange::Graphics::RenderContext& ctx);

