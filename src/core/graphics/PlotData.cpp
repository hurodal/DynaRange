// File: src/core/graphics/PlotData.cpp
/**
 * @file src/core/graphics/PlotData.cpp
 * @brief Implements the low-level Cairo drawing functions for the dynamic plot data.
 */
#include "PlotData.hpp"
#include "PlotBase.hpp"
#include "Colour.hpp" 
#include "../math/Math.hpp"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Anonymous namespace for helper functions in this file
namespace { 

/**
 * @brief Maps data coordinates (EV, dB) to pixel coordinates on the plot.
 * @param ev Exposure value (x-axis).
 * @param db SNR in dB (y-axis).
 * @param bounds Map containing plot boundaries.
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
 * @brief Draws a single SNR curve (polynomial fit) for one curve.
 * @param cr The Cairo context.
 * @param curve The curve data (coefficients, EV, dB).
 * @param bounds Plot boundaries.
 */
void DrawCurve(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    // Curve line: Red
    PlotColors::cairo_set_source_red(cr);
    cairo_set_line_width(cr, 2.0);

    // Evaluate polynomial at start point
    double snr_poly_start = 0.0;
    for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
        snr_poly_start += curve.poly_coeffs.at<double>(j) * std::pow(curve.signal_ev.front(), curve.poly_coeffs.rows - 1 - j);
    }
    auto [start_x, start_y] = map_coords(curve.signal_ev.front(), snr_poly_start);
    cairo_move_to(cr, start_x, start_y);

    // Sample points along the curve between min and max EV
    for (double ev = curve.signal_ev.front(); ev <= curve.signal_ev.back(); ev += 0.05) {
        double snr_poly = 0.0;
        for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
            snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j);
        }
        auto [px, py] = map_coords(ev, snr_poly);
        cairo_line_to(cr, px, py);
    }
    cairo_stroke(cr);
}

/**
 * @brief Draws all data points (blue circles) for one curve.
 * @param cr The Cairo context.
 * @param curve The curve data (EV, dB).
 * @param bounds Plot boundaries.
 */
void DrawDataPoints(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    // Data points: Blue
    PlotColors::cairo_set_source_blue(cr);
    for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
        auto [px, py] = map_coords(curve.signal_ev[j], curve.snr_db[j]);
        cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

/**
 * @brief Draws the curve label (e.g., "ISO 200") near the end of the curve.
 * @param cr The Cairo context.
 * @param curve The curve data.
 * @param bounds Plot boundaries.
 */
void DrawCurveLabel(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    std::string label = curve.plot_label;
    auto [label_x, label_y] = map_coords(curve.signal_ev.back(), curve.snr_db.back());

    // Curve label: Same color as curve (Red)
    PlotColors::cairo_set_source_red(cr);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_move_to(cr, label_x - 40, label_y - 30);
    cairo_show_text(cr, label.c_str());
}

/**
 * @brief Draws an intersection point with a given SNR threshold and its label.
 * @param cr The Cairo context.
 * @param curve The curve data.
 * @param bounds Plot boundaries.
 * @param threshold_db The SNR threshold in dB to find the intersection for.
 * @param label_prefix Text prefix for the label ("12dB" or "0dB").
 * @param draw_above Flag to alternate label position (up/down).
 */
void DrawThresholdIntersection(
    cairo_t* cr,
    const CurveData& curve,
    const std::map<std::string, double>& bounds,
    double threshold_db,
    const char* label_prefix,
    bool& draw_above)
{
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    auto min_max_ev = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
    double min_ev = *min_max_ev.first;
    double max_ev = *min_max_ev.second;

    auto ev_opt = FindIntersectionEV(curve.poly_coeffs, threshold_db, min_ev, max_ev);
    if (!ev_opt) return;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << *ev_opt << "EV";

    auto [px, py] = map_coords(*ev_opt, threshold_db);

    // Alternate label position vertically
    double offset_x = draw_above ? 25.0 : 15.0;
    double offset_y = draw_above ? -10.0 : 15.0;

    // Label text: Black
    PlotColors::cairo_set_source_black(cr);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    cairo_move_to(cr, px + offset_x, py + offset_y);
    cairo_show_text(cr, ss.str().c_str());

    draw_above = !draw_above; // Toggle for next curve
}

} // end of anonymous namespace

// ================== PUBLIC FUNCTION ==================

void DrawCurvesAndData(
    cairo_t* cr,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds)
{
    // Using a boolean to alternate is simpler and more robust.
    bool draw_above_12db = true;
    bool draw_above_0db = true;

    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        
        // Draw each component separately
        DrawCurve(cr, curve, bounds);
        DrawDataPoints(cr, curve, bounds);
        DrawCurveLabel(cr, curve, bounds);

        // Draw threshold intersections
        DrawThresholdIntersection(cr, curve, bounds, 12.0, "12dB", draw_above_12db);
        DrawThresholdIntersection(cr, curve, bounds, 0.0, "0dB", draw_above_0db);
    }
}