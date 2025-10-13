// File: src/core/graphics/PlotBase.hpp
/**
 * @file src/core/graphics/PlotBase.hpp
 * @brief Declares the function to draw the static base of a plot (axes, grid, titles, etc.).
 */
#pragma once
#include "../arguments/ArgumentsOptions.hpp" // For RawChannelSelection
#include "RenderContext.hpp"
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>

constexpr int MARGIN_LEFT = 180;
constexpr int MARGIN_RIGHT = 100;
constexpr int MARGIN_TOP = 100;
constexpr int MARGIN_BOTTOM = 120;

inline std::pair<double, double> MapToPixelCoords(double ev, double db, const std::map<std::string, double>& bounds, const DynaRange::Graphics::RenderContext& ctx) {
    const int plot_area_width = ctx.base_width - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = ctx.base_height - MARGIN_TOP - MARGIN_BOTTOM;

    double px = MARGIN_LEFT + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
    double py = (ctx.base_height - MARGIN_BOTTOM) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
    return std::make_pair(px, py);
}

/**
 * @brief Draws the static base of a plot (background, grid, axes, titles, etc.).
 * @param cr The cairo drawing context.
 * @param ctx The rendering context with canvas dimensions.
 * @param title The main title of the plot.
 * @param channels The raw channel selection, used for display purposes in the subtitle.
 * @param bounds A map containing the plot boundaries.
 * @param command_text The command-line text to display at the bottom of the plot.
 * @param snr_thresholds A vector of SNR thresholds to draw as horizontal dashed lines.
 */
void DrawPlotBase(
    cairo_t* cr,
    const DynaRange::Graphics::RenderContext& ctx,
    const std::string& title,
    const RawChannelSelection& channels,
    const std::map<std::string, double>& bounds,
    const std::string& command_text,
    const std::vector<double>& snr_thresholds);