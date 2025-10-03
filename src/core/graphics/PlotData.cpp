// File: src/core/graphics/PlotData.cpp
/**
 * @file src/core/graphics/PlotData.cpp
 * @brief Implements the low-level Cairo drawing functions for the dynamic plot data.
 */
#include "PlotData.hpp"
#include "PlotBase.hpp"
#include "Colour.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <set>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Anonymous namespace for internal helper functions
namespace {

void DrawCurve(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    if (curve.curve_points.empty()) {
        return;
    }
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    PlotColors::SetSourceFromChannel(cr, curve.channel);
    cairo_set_line_width(cr, 2.0);

    const auto& first_point = curve.curve_points[0];
    auto [start_x, start_y] = map_coords(first_point.first, first_point.second);
    cairo_move_to(cr, start_x, start_y);

    for (size_t i = 1; i < curve.curve_points.size(); ++i) {
        const auto& point = curve.curve_points[i];
        auto [x, y] = map_coords(point.first, point.second);
        cairo_line_to(cr, x, y);
    }
    cairo_stroke(cr);
}

void DrawDataPoints(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    // If the main curve is AVG, color each point by its original source.
    // Otherwise, all points have the same color as the curve.
    if (curve.channel == DataSource::AVG) {
        for (const auto& point : curve.points) {
            PlotColors::SetSourceFromChannel(cr, point.channel);
            auto [px, py] = map_coords(point.ev, point.snr_db);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    } else {
        PlotColors::SetSourceFromChannel(cr, curve.channel);
        for (const auto& point : curve.points) {
            auto [px, py] = map_coords(point.ev, point.snr_db);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
}

void DrawCurveLabel(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds) {
    if (curve.points.empty()) return;

    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    std::string label = curve.plot_label;
    
    // Find the point with the maximum EV to place the label.
    auto max_ev_point_it = std::max_element(curve.points.begin(), curve.points.end(),
        [](const PointData& a, const PointData& b){ return a.ev < b.ev; });
    
    auto [label_x, label_y] = map_coords(max_ev_point_it->ev, max_ev_point_it->snr_db);
    
    PlotColors::cairo_set_source_black(cr);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_move_to(cr, label_x - 40, label_y - 30);
    cairo_show_text(cr, label.c_str());
}

void DrawThresholdIntersection(cairo_t* cr,
                               const CurveData& curve,
                               const DynamicRangeResult& dr_result,
                               const std::map<std::string, double>& bounds,
                               double threshold_db)
{
    if (curve.points.empty()) return;

    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    if (dr_result.dr_values_ev.count(threshold_db)) {
        double dr_value = dr_result.dr_values_ev.at(threshold_db);
        if (dr_value <= 0) return;

        double ev_at_threshold = -dr_value;

        auto min_max_ev_it = std::minmax_element(curve.points.begin(), curve.points.end(),
            [](const PointData& a, const PointData& b){ return a.ev < b.ev; });
        
        if (ev_at_threshold < min_max_ev_it.first->ev || ev_at_threshold > min_max_ev_it.second->ev) {
            return;
        }

        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << dr_value << "EV";
        auto [px, py] = map_coords(ev_at_threshold, threshold_db);

        double vertical_offset = 0.0;
        double horizontal_offset = 40.0;

        switch (curve.channel) {
            case DataSource::AVG: vertical_offset = -8.0;  horizontal_offset += 20.0; break;
            case DataSource::B:   vertical_offset = -18.0; horizontal_offset += 15.0; break;
            case DataSource::G2:  vertical_offset = -28.0; horizontal_offset += 10.0; break;
            case DataSource::G1:  vertical_offset = -38.0; horizontal_offset += 5.0;  break;
            case DataSource::R:   vertical_offset = -48.0; horizontal_offset += 0.0;  break;
        }

        PlotColors::SetSourceFromChannel(cr, curve.channel);
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        cairo_move_to(cr, px + horizontal_offset, py + vertical_offset);
        cairo_show_text(cr, ss.str().c_str());
    }
}

} // end anonymous namespace

void DrawCurvesAndData(cairo_t* cr,
                       const PlotInfoBox& info_box,
                       const std::vector<CurveData>& curves,
                       const std::vector<DynamicRangeResult>& results,
                       const std::map<std::string, double>& bounds) {
    info_box.Draw(cr);
    
    // --- PASS 1: Draw all curves and data points ---
    for (const auto& curve : curves) {
        if (curve.points.empty()) continue;
        DrawCurve(cr, curve, bounds);
        DrawDataPoints(cr, curve, bounds);
    }

    // --- PASS 2: Draw all labels on top of the curves and points ---
    std::set<std::string> drawn_iso_labels;
    for (size_t i = 0; i < curves.size(); ++i) {
        const auto& curve = curves[i];
        if (curve.points.empty()) continue;

        if (drawn_iso_labels.find(curve.plot_label) == drawn_iso_labels.end()) {
            DrawCurveLabel(cr, curve, bounds);
            drawn_iso_labels.insert(curve.plot_label);
        }

        if (i < results.size()) {
            const auto& result = results[i];
            DrawThresholdIntersection(cr, curve, result, bounds, 12.0);
            DrawThresholdIntersection(cr, curve, result, bounds, 0.0);
        }
    }
}