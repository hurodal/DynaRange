// File: src/core/graphics/PlotBase.hpp
/**
 * @file src/core/graphics/PlotBase.hpp
 * @brief Declares the function to draw the static base of a plot (axes, grid, titles, etc.).
 */
#pragma once
#include "../arguments/ArgumentsOptions.hpp"
#include "../Constants.hpp" 
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>

// --- Centralized Plot Dimension Constants ---
namespace PlotDefs {
    constexpr int BASE_WIDTH = 1920;
    constexpr int BASE_HEIGHT = 1080;

    // Determine if the output format is a vector format.
    constexpr bool IS_VECTOR = (DynaRange::Constants::PLOT_FORMAT == DynaRange::Constants::PlotOutputFormat::PDF ||
                                DynaRange::Constants::PLOT_FORMAT == DynaRange::Constants::PlotOutputFormat::SVG);

    // Apply scaling factor only for vector formats.
    constexpr double SCALE = IS_VECTOR ? DynaRange::Constants::VECTOR_PLOT_SCALE_FACTOR : 1.0;

    constexpr int WIDTH = static_cast<int>(BASE_WIDTH * SCALE);
    constexpr int HEIGHT = static_cast<int>(BASE_HEIGHT * SCALE);
}


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
 * @return Pair of pixel coordinates (x, y).
 * @details This function is defined as 'inline' to allow safe inclusion from multiple .cpp files.
 */
inline std::pair<double, double> MapToPixelCoords(double ev, double db, const std::map<std::string, double>& bounds) {
    // This function now uses the BASE dimensions, as it operates before the global cairo_scale.
    const int plot_area_width = PlotDefs::BASE_WIDTH - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = PlotDefs::BASE_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;
    double px = MARGIN_LEFT + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
    double py = (PlotDefs::BASE_HEIGHT - MARGIN_BOTTOM) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
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