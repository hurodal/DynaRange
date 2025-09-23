// File: src/core/graphics/PlotData.cpp
/**
 * @file src/core/graphics/PlotData.cpp
 * @brief Implements the low-level Cairo drawing functions for the dynamic plot data.
 */
#include "PlotData.hpp"
#include "PlotBase.hpp" // <-- Ahora incluye PlotBase.hpp, que contiene MapToPixelCoords
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
 * @brief Draws a single SNR curve (polynomial fit) for one curve.
 * @param cr The Cairo context.
 * @param curve The curve data (coefficients, EV, dB).
 * @param bounds Plot boundaries.
 */
void DrawCurve(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    // Si no hay datos, no dibujamos nada para esta curva.
    if (curve.snr_db.empty()) {
        return;
    }

    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };
    PlotColors::cairo_set_source_red(cr);
    cairo_set_line_width(cr, 2.0);

    // 1. Obtenemos el rango real de los datos de SNR para ESTA curva.
    auto min_max_snr = std::minmax_element(curve.snr_db.begin(), curve.snr_db.end());
    double min_db_data = *min_max_snr.first;
    double max_db_data = *min_max_snr.second;

    // 2. Calculamos el punto de inicio de la curva en el límite inferior de los datos.
    double start_ev_poly = 0.0;
    for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
        start_ev_poly += curve.poly_coeffs.at<double>(j) * std::pow(min_db_data, curve.poly_coeffs.rows - 1 - j);
    }
    auto [start_x, start_y] = map_coords(start_ev_poly, min_db_data);
    cairo_move_to(cr, start_x, start_y);

    // 3. Iteramos únicamente dentro del rango de los datos reales.
    for (double db = min_db_data; db <= max_db_data; db += 0.1) {
        double ev_poly = 0.0;
        // Evalúa el polinomio EV = f(SNR_dB)
        for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
            ev_poly += curve.poly_coeffs.at<double>(j) * std::pow(db, curve.poly_coeffs.rows - 1 - j);
        }
        auto [px, py] = map_coords(ev_poly, db);
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
    // Usamos MapToPixelCoords desde PlotBase.hpp
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
    // Usamos MapToPixelCoords desde PlotBase.hpp
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
    // Usamos MapToPixelCoords desde PlotBase.hpp
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };
    auto min_max_ev = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
    double min_ev = *min_max_ev.first;
    double max_ev = *min_max_ev.second;
    auto ev_opt = FindIntersectionEV(curve.poly_coeffs, threshold_db);
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
    const PlotInfoBox& info_box,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds)
{
    // Dibuja la caja de información primero
    info_box.Draw(cr);
    
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