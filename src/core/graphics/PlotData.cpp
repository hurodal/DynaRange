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
    cairo_move_to(cr, start_x, start_y);

    // Iterate over the SNR range to draw the curve EV = f(SNR_dB).
    for (double snr_db = min_snr_data; snr_db <= max_snr_data; snr_db += 0.1) {
        // Evaluate the polynomial EV = f(SNR_dB)
        double ev_poly = EvaluatePolynomial(curve.poly_coeffs, snr_db);
        auto [px, py] = map_coords(ev_poly, snr_db);
        cairo_line_to(cr, px, py);
    }
    cairo_stroke(cr);
}

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

// This function existed previously and has been modified.
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

    // Directly calculate EV from the polynomial EV = f(SNR_dB).
    double ev_at_threshold = EvaluatePolynomial(curve.poly_coeffs, threshold_db);
    
    // Check if the calculated EV is within the data's original range to avoid extrapolation artifacts.
    auto min_max_ev = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
    if (ev_at_threshold < *min_max_ev.first || ev_at_threshold > *min_max_ev.second) {
        return; // Don't draw intersection if it's outside the data range.
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << ev_at_threshold << "EV";
    auto [px, py] = map_coords(ev_at_threshold, threshold_db);

    // Alternate label position vertically and horizontally
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

void DrawCurvesAndData(
    cairo_t* cr,
    const PlotInfoBox& info_box,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds)
{
    info_box.Draw(cr);
    bool draw_above_12db = true;
    bool draw_above_0db = true;
    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        
        DrawCurve(cr, curve, bounds);
        DrawDataPoints(cr, curve, bounds);
        DrawCurveLabel(cr, curve, bounds);
        
        // Draw threshold intersections for default thresholds if they exist
        DrawThresholdIntersection(cr, curve, bounds, 12.0, "12dB", draw_above_12db);
        DrawThresholdIntersection(cr, curve, bounds, 0.0, "0dB", draw_above_0db);
    }
}