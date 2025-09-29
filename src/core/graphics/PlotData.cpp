// File: src/core/graphics/PlotData.cpp
/**
 * @file src/core/graphics/PlotData.cpp
 * @brief Implements the low-level Cairo drawing functions for the dynamic plot data.
 */
#include "PlotData.hpp"
#include "PlotBase.hpp"
#include "Colour.hpp"
#include "../math/Math.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Anonymous namespace for helper functions in this file
namespace {

/**
 * @brief Draws a single SNR curve using a polynomial fit.
 * @param cr The cairo drawing context.
 * @param curve The CurveData struct containing the curve's data and coefficients.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 */
void DrawCurve(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    if (curve.snr_db.empty()) {
        return;
    }

    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    PlotColors::cairo_set_source_red(cr);
    cairo_set_line_width(cr, 2.0);

    // Get the min/max range of the SNR data for this curve.
    auto min_max_snr = std::minmax_element(curve.snr_db.begin(), curve.snr_db.end());
    double min_snr_data = *min_max_snr.first;
    double max_snr_data = *min_max_snr.second;

    // Evaluate the starting point of the curve.
    double start_ev_poly = EvaluatePolynomial(curve.poly_coeffs, min_snr_data);
    auto [start_x, start_y] = map_coords(start_ev_poly, min_snr_data);

    // Start the path at the first point.
    cairo_move_to(cr, start_x, start_y);

    // Generate a series of points along the curve by evaluating the polynomial
    // over a dense grid of SNR values.
    const int NUM_POINTS = 100; // Use 100 points for smoothness.
    for (int i = 1; i <= NUM_POINTS; ++i) {
        double snr_step = min_snr_data + (i * (max_snr_data - min_snr_data) / NUM_POINTS);
        double ev_at_snr = EvaluatePolynomial(curve.poly_coeffs, snr_step);
        auto [x, y] = map_coords(ev_at_snr, snr_step);
        cairo_line_to(cr, x, y);
    }

    // Stroke the entire path to draw the curve.
    cairo_stroke(cr);
}

/**
 * @brief Draws the individual data points for a curve as small blue circles.
 * @param cr The cairo drawing context.
 * @param curve The CurveData struct containing the curve's data.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 */
void DrawDataPoints(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    PlotColors::cairo_set_source_blue(cr);
    for (size_t j = 0; j < curve.signal_ev.size(); ++j) {
        auto [px, py] = map_coords(curve.signal_ev[j], curve.snr_db[j]);
        cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

/**
 * @brief Draws the label for a curve at its endpoint.
 * @param cr The cairo drawing context.
 * @param curve The CurveData struct containing the curve's data.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
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
 * @brief Draws an intersection point with a threshold line and labels it.
 * @param cr The cairo drawing context.
 * @param curve The CurveData struct containing the curve's data.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 * @param threshold_db The SNR threshold value (e.g., 12.0).
 * @param label_prefix A string prefix for the label (e.g., "12dB").
 * @param draw_above A boolean flag to alternate the vertical position of the label.
 */
void DrawThresholdIntersection(cairo_t* cr,
                               const CurveData& curve,
                               const std::map<std::string, double>& bounds,
                               double threshold_db,
                               const char* label_prefix,
                               bool& draw_above) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    // Directly calculate EV from the polynomial: EV = f(SNR_dB).
    double ev_at_threshold = EvaluatePolynomial(curve.poly_coeffs, threshold_db);

    // Check if the calculated EV is within the data range.
    auto min_max_ev = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;

    if (ev_at_threshold < min_ev_data || ev_at_threshold > max_ev_data) {
        return; // Don't draw intersection if it's outside the data range.
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << ev_at_threshold << "EV";
    auto [px, py] = map_coords(ev_at_threshold, threshold_db);

    // Alternate label position vertically and horizontally.
    double offset_x = draw_above ? 25.0 : 15.0;
    double offset_y = draw_above ? -10.0 : 15.0;

    // Label text: Black
    PlotColors::cairo_set_source_black(cr);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    cairo_move_to(cr, px + offset_x, py + offset_y);
    cairo_show_text(cr, ss.str().c_str());
    draw_above = !draw_above; // Toggle for next curve.
}

} // end of anonymous namespace

/**
 * @brief Draws all the dynamic data onto the plot: curves, points, labels, and intersections.
 * @param cr The cairo drawing context.
 * @param info_box An object containing information to display in a box on the plot.
 * @param curves A vector of CurveData structs, each representing a curve to draw.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 */
void DrawCurvesAndData(cairo_t* cr,
                       const PlotInfoBox& info_box,
                       const std::vector<CurveData>& curves,
                       const std::map<std::string, double>& bounds) {
    info_box.Draw(cr);
    bool draw_above_12db = true;
    bool draw_above_0db = true;
    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        DrawCurve(cr, curve, bounds); // This now works!
        DrawDataPoints(cr, curve, bounds);
        DrawCurveLabel(cr, curve, bounds);
        // Draw threshold intersections for default thresholds if they exist.
        DrawThresholdIntersection(cr, curve, bounds, 12.0, "12dB", draw_above_12db);
        DrawThresholdIntersection(cr, curve, bounds, 0.0, "0dB", draw_above_0db);
    }
}