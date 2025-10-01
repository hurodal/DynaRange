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
    if (curve.snr_db.empty() || curve.poly_coeffs.empty()) {
        return;
    }

    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    PlotColors::cairo_set_source_red(cr);
    cairo_set_line_width(cr, 2.0);

    // Iterate over the independent variable (EV) range, not the SNR range.
    auto min_max_ev = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;

    // Start the path at the first evaluated point of the polynomial.
    double start_snr_poly = EvaluatePolynomial(curve.poly_coeffs, min_ev_data);
    auto [start_x, start_y] = map_coords(min_ev_data, start_snr_poly);
    cairo_move_to(cr, start_x, start_y);

    // Generate a series of points along the curve by evaluating SNR = f(EV).
    const int NUM_POINTS = 100;
    for (int i = 1; i <= NUM_POINTS; ++i) {
        double ev_step = min_ev_data + (i * (max_ev_data - min_ev_data) / NUM_POINTS);
        double snr_at_ev = EvaluatePolynomial(curve.poly_coeffs, ev_step);
        auto [x, y] = map_coords(ev_step, snr_at_ev);
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
 * @param dr_result The dynamic range results, used for getting the intersection EV value.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 * @param threshold_db The SNR threshold value (e.g., 12.0).
 * @param draw_above A boolean flag to alternate the vertical position of the label.
 */
void DrawThresholdIntersection(cairo_t* cr,
                               const CurveData& curve,
                               const DynamicRangeResult& dr_result,
                               const std::map<std::string, double>& bounds,
                               double threshold_db,
                               bool& draw_above) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    // Use the pre-calculated DR value instead of trying to re-calculate it.
    if (dr_result.dr_values_ev.count(threshold_db)) {
        double dr_value = dr_result.dr_values_ev.at(threshold_db);
        if (dr_value <= 0) return;

        double ev_at_threshold = -dr_value; // by definition, DR = -EV

        // Check if the intersection is within the plotted data range before drawing.
        auto min_max_ev = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        if (curve.signal_ev.empty() || ev_at_threshold < *min_max_ev.first || ev_at_threshold > *min_max_ev.second) {
            return;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << dr_value << "EV";
        auto [px, py] = map_coords(ev_at_threshold, threshold_db);

        double offset_x = 25.0; 
        double offset_y = draw_above ? -10.0 : 20.0;

        // Label text: Black
        PlotColors::cairo_set_source_black(cr);
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        cairo_move_to(cr, px + offset_x, py + offset_y);
        cairo_show_text(cr, ss.str().c_str());
        draw_above = !draw_above;
    }
}

} // end of anonymous namespace

/**
 * @brief Draws all the dynamic data onto the plot: curves, points, labels, and intersections.
 * @param cr The cairo drawing context.
 * @param info_box An object containing information to display in a box on the plot.
 * @param curves A vector of CurveData structs, each representing a curve to draw.
 * @param results A vector of DynamicRangeResult, containing DR data for labels.
 * @param bounds A map containing the plot boundaries to correctly map data coordinates.
 */
void DrawCurvesAndData(cairo_t* cr,
                       const PlotInfoBox& info_box,
                       const std::vector<CurveData>& curves,
                       const std::vector<DynamicRangeResult>& results,
                       const std::map<std::string, double>& bounds) {
    info_box.Draw(cr);
    bool draw_above_12db = true;
    bool draw_above_0db = true;

    for (size_t i = 0; i < curves.size(); ++i) {
        const auto& curve = curves[i];
        if (curve.signal_ev.empty()) continue;

        DrawCurve(cr, curve, bounds);
        DrawDataPoints(cr, curve, bounds);
        DrawCurveLabel(cr, curve, bounds);

        if (i < results.size()) {
            const auto& result = results[i];
            DrawThresholdIntersection(cr, curve, result, bounds, 12.0, draw_above_12db);
            DrawThresholdIntersection(cr, curve, result, bounds, 0.0, draw_above_0db);
        }
    }
}