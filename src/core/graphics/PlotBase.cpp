// File: src/core/graphics/PlotBase.cpp
/**
 * @file src/core/graphics/PlotBase.cpp
 * @brief Implements the low-level Cairo drawing functions for the plot base.
 */
#include "PlotBase.hpp"
#include "Colour.hpp" 
#include <cmath>
#include <iomanip>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Anonymous namespace for helper functions in this file
namespace { 

/**
 * @brief Draws a dashed line on the cairo context.
 * @param cr The cairo drawing context.
 * @param x1 Starting x-coordinate.
 * @param y1 Starting y-coordinate.
 * @param x2 Ending x-coordinate.
 * @param y2 Ending y-coordinate.
 * @param dash_length The length of each dash segment.
 */
void DrawDashedLine(cairo_t* cr, double x1, double y1, double x2, double y2, double dash_length = 20.0) {
    double dashes[] = {dash_length, dash_length};
    cairo_save(cr);
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
    cairo_restore(cr);
}

/**
 * @brief Maps data coordinates (EV, dB) to pixel coordinates on the plot.
 * @param ev Exposure value (x-axis).
 * @param db SNR in dB (y-axis).
 * @param bounds Map containing plot boundaries (min_ev, max_ev, min_db, max_db).
 * @return Pair of pixel coordinates (x, y).
 */
std::pair<double, double> MapToPixelCoords(double ev, double db, const std::map<std::string, double>& bounds) {
    const int plot_area_width = PLOT_WIDTH - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = PLOT_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;

    double px = MARGIN_LEFT + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
    double py = (PLOT_HEIGHT - MARGIN_BOTTOM) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
    return std::make_pair(px, py);
}

/**
 * @brief Draws the background and plot border.
 * @param cr The cairo drawing context.
 * @param bounds Map containing the plot boundaries.
 */
void DrawPlotBackgroundAndBorder(cairo_t* cr, const std::map<std::string, double>& bounds) {
    const int plot_area_width = PLOT_WIDTH - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = PLOT_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;

    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    // Background: White
    PlotColors::cairo_set_source_white(cr);
    cairo_rectangle(cr, 0, 0, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_fill(cr);

    // Plot border: Black
    PlotColors::cairo_set_source_black(cr);
    cairo_set_line_width(cr, 3.0);
    cairo_rectangle(cr, MARGIN_LEFT, MARGIN_TOP, plot_area_width, plot_area_height);
    cairo_stroke(cr);
}

/**
 * @brief Draws the grid lines (vertical for EV, horizontal for dB).
 * @param cr The cairo drawing context.
 * @param bounds Map containing the plot boundaries.
 */
void DrawGridLines(cairo_t* cr, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    // Grid lines: Dark gray (GREY_20) — matches your original "black" appearance
    PlotColors::cairo_set_source_grey_20(cr);
    cairo_set_line_width(cr, 1.0);

    // Vertical grid lines (EV)
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        auto [p1x, p1y] = map_coords(ev, bounds.at("min_db"));
        auto [p2x, p2y] = map_coords(ev, bounds.at("max_db"));
        cairo_move_to(cr, p1x, p1y); cairo_line_to(cr, p2x, p2y); cairo_stroke(cr);
    }

    // Horizontal grid lines (dB)
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), db);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), db);
        cairo_move_to(cr, p1x, p1y); cairo_line_to(cr, p2x, p2y); cairo_stroke(cr);
    }
}

/**
 * @brief Draws the dashed horizontal lines for SNR thresholds and their labels.
 * @param cr The cairo drawing context.
 * @param bounds Map containing the plot boundaries.
 * @param snr_thresholds Vector of SNR thresholds in dB.
 */
void DrawThresholdLines(cairo_t* cr, const std::map<std::string, double>& bounds, const std::vector<double>& snr_thresholds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    cairo_set_line_width(cr, 2.0);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 16.0);

    for(const double threshold : snr_thresholds) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), threshold);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), threshold);
        DrawDashedLine(cr, p1x, p1y, p2x, p2y);

        std::stringstream ss;
        ss << "SNR > " << std::fixed << std::setprecision(1) << threshold << "dB";
        cairo_move_to(cr, p1x + 20, p1y - 10);
        cairo_show_text(cr, ss.str().c_str());
    }
}

/**
 * @brief Draws the tick labels on the X-axis (EV values).
 * @param cr The cairo drawing context.
 * @param bounds Map containing the plot boundaries.
 */
void DrawXAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    cairo_set_font_size(cr, 16.0);
    cairo_text_extents_t extents;

    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) { 
        std::string ev_str = std::to_string((int)ev);
        cairo_text_extents(cr, ev_str.c_str(), &extents);
        auto [px, py] = map_coords(ev, bounds.at("min_db"));
        cairo_move_to(cr, px - extents.width / 2, PLOT_HEIGHT - MARGIN_BOTTOM + 25); 
        cairo_show_text(cr, ev_str.c_str());
    }
}

/**
 * @brief Draws the tick labels on the Y-axis (dB values).
 * @param cr The cairo drawing context.
 * @param bounds Map containing the plot boundaries.
 */
void DrawYAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    cairo_set_font_size(cr, 16.0);
    cairo_text_extents_t extents;

    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) { 
        std::string db_str = std::to_string((int)db);
        cairo_text_extents(cr, db_str.c_str(), &extents);
        auto [px, py] = map_coords(bounds.at("min_ev"), db);
        cairo_move_to(cr, MARGIN_LEFT - extents.width - 15, py + extents.height / 2); 
        cairo_show_text(cr, db_str.c_str());
    }
}

/**
 * @brief Draws the main title, axis labels, and command text.
 * @param cr The cairo drawing context.
 * @param title The main title of the plot.
 * @param command_text The command-line text to display at the bottom of the plot.
 */
void DrawPlotAnnotations(cairo_t* cr, const std::string& title, const std::string& command_text) {
    cairo_text_extents_t extents;

    // Title: Black
    PlotColors::cairo_set_source_black(cr);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24.0);
    cairo_text_extents(cr, title.c_str(), &extents);
    cairo_move_to(cr, PLOT_WIDTH / 2 - extents.width / 2, MARGIN_TOP - 40);
    cairo_show_text(cr, title.c_str());

    // X-axis label: Black
    PlotColors::cairo_set_source_black(cr);
    cairo_set_font_size(cr, 20.0);
    std::string x_label = "RAW exposure (EV)";
    cairo_text_extents(cr, x_label.c_str(), &extents);
    cairo_move_to(cr, PLOT_WIDTH / 2 - extents.width / 2, PLOT_HEIGHT - MARGIN_BOTTOM + 70);
    cairo_show_text(cr, x_label.c_str());

    // Y-axis label (rotated): Black
    std::string y_label = "SNR (dB)";
    cairo_text_extents(cr, y_label.c_str(), &extents);
    cairo_save(cr);
    cairo_move_to(cr, MARGIN_LEFT / 2.0 - extents.height / 2.0, PLOT_HEIGHT / 2.0 + extents.width / 2.0);
    cairo_rotate(cr, -M_PI / 2.0);
    cairo_show_text(cr, y_label.c_str());
    cairo_restore(cr);

    // Command text (bottom right): Medium gray (GREY_50)
    if (!command_text.empty()) {
        PlotColors::cairo_set_source_grey_50(cr);
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        cairo_text_extents_t cmd_extents;
        cairo_text_extents(cr, command_text.c_str(), &cmd_extents);
        cairo_move_to(cr, PLOT_WIDTH - MARGIN_RIGHT - cmd_extents.width - 10, PLOT_HEIGHT - 20);
        cairo_show_text(cr, command_text.c_str());
    }
}

} // end of anonymous namespace

// ================== PUBLIC FUNCTION ==================

void DrawPlotBase(
    cairo_t* cr,
    const std::string& title,
    const std::map<std::string, double>& bounds,
    const std::string& command_text,
    const std::vector<double>& snr_thresholds)
{
    // Draw all components in logical order
    DrawPlotBackgroundAndBorder(cr, bounds);
    DrawGridLines(cr, bounds);
    DrawThresholdLines(cr, bounds, snr_thresholds);
    DrawXAxisLabels(cr, bounds);
    DrawYAxisLabels(cr, bounds);
    DrawPlotAnnotations(cr, title, command_text);
}