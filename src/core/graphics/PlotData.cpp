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
#include <set>
#include <optional>
#include <tuple>

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

    // Reduced offsets to bring label closer to the curve's end point.
    cairo_move_to(cr, label_x + 10, label_y - 15);
    cairo_show_text(cr, label.c_str());
}

void DrawThresholdIntersection(cairo_t* cr,
                               const std::string& text_to_draw,
                               DataSource channel,
                               double primary_px,
                               double primary_py,
                               double primary_angle_rad,
                               int iso_index,
                               int channel_index,
                               int group_size)
{
    // --- 1. Calculate Position Offset relative to the center of the block ---
    // Reduced LINE_HEIGHT to decrease spacing within subgroups.
    const double LINE_HEIGHT = 12.0;
    const double H_OFFSET_FROM_BASE_LINE = 0.0;
    
    const double BASE_GAP = 20.0;
    const double GAP_PER_LABEL = 1.5;
    double dynamic_gap = BASE_GAP + (group_size * GAP_PER_LABEL);

    double v_offset_from_center;
    if (group_size == 1) {
        v_offset_from_center = -LINE_HEIGHT; 
    } else {
        int labels_above = (group_size == 5) ? 3 : static_cast<int>(std::ceil(static_cast<double>(group_size) / 2.0));

        if (channel_index < labels_above) {
            int pos = labels_above - 1 - channel_index;
            v_offset_from_center = -(pos * LINE_HEIGHT + dynamic_gap / 2.0);
        } else {
            int pos = channel_index - labels_above;
            v_offset_from_center = pos * LINE_HEIGHT + dynamic_gap / 2.0;
        }
    }
    
    // --- 2. Draw using the shared "base line" geometry ---
    cairo_save(cr);
    PlotColors::SetSourceFromChannel(cr, channel);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    cairo_translate(cr, primary_px, primary_py);
    cairo_rotate(cr, primary_angle_rad);
    
    cairo_move_to(cr, H_OFFSET_FROM_BASE_LINE, v_offset_from_center);
    cairo_show_text(cr, text_to_draw.c_str());
    
    cairo_restore(cr);
}

} // end anonymous namespace

void DrawCurvesAndData(cairo_t* cr,
                       const PlotInfoBox& info_box,
                       const std::vector<CurveData>& curves,
                       const std::vector<DynamicRangeResult>& results,
                       const std::map<std::string, double>& bounds)
{
    info_box.Draw(cr);
    // --- PASS 1: Draw curves and points ---
    for (const auto& curve : curves) {
        if (curve.points.empty()) continue;
        DrawCurve(cr, curve, bounds);
        DrawDataPoints(cr, curve, bounds);
    }

    // --- PASS 2: Draw labels ---
    std::set<std::string> drawn_iso_labels;
    
    // Group curves by filename (ISO) to handle them as a block
    std::map<std::string, std::vector<const CurveData*>> curves_by_iso;
    const std::vector<DataSource> canonical_order = {DataSource::R, DataSource::G1, DataSource::G2, DataSource::B, DataSource::AVG};
    for (DataSource ch : canonical_order) {
        for(const auto& c : curves) {
            if (c.channel == ch) {
                curves_by_iso[c.filename].push_back(&c);
            }
        }
    }

    int iso_index = 0;
    for (const auto& pair : curves_by_iso) {
        const auto& iso_curves_group = pair.second;
        if (iso_curves_group.empty()) continue;

        // 1. Identify the primary curve and draw its main label
        const CurveData& primary_curve = *iso_curves_group[0];
        if (drawn_iso_labels.find(primary_curve.plot_label) == drawn_iso_labels.end()) {
            DrawCurveLabel(cr, primary_curve, bounds);
            drawn_iso_labels.insert(primary_curve.plot_label);
        }

        // 2. Calculate the "base line" geometry (position and angle) ONCE from the primary curve
        auto calculate_base_geometry = [&](double threshold) -> std::optional<std::tuple<double, double, double>> {
            auto it = std::find_if(results.begin(), results.end(),
                [&](const DynamicRangeResult& r){ return r.filename == primary_curve.filename && r.channel == primary_curve.channel; });

            if (it != results.end() && it->dr_values_ev.count(threshold)) {
                double dr_value = it->dr_values_ev.at(threshold);
                if (dr_value <= 0) return std::nullopt;

                double ev = -dr_value;
                auto [px, py] = MapToPixelCoords(ev, threshold, bounds);
                
                double slope = EvaluatePolynomialDerivative(primary_curve.poly_coeffs, ev);
                const double plot_w = PlotDefs::BASE_WIDTH - MARGIN_LEFT - MARGIN_RIGHT;
                const double plot_h = PlotDefs::BASE_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;
                const double range_ev = bounds.at("max_ev") - bounds.at("min_ev");
                const double range_db = bounds.at("max_db") - bounds.at("min_db");
                double slope_px = -slope * (plot_h / range_db) / (plot_w / range_ev);
                double angle = std::atan(slope_px);

                return {{px, py, angle}};
            }
            return std::nullopt;
        };

        auto base_geom_12db = calculate_base_geometry(12.0);
        auto base_geom_0db  = calculate_base_geometry(0.0);
        
        // 3. Draw all labels in the group using the shared base line geometry
        int group_size = iso_curves_group.size();
        for (int i = 0; i < group_size; ++i) {
            const CurveData& current_curve = *iso_curves_group[i];
            auto it = std::find_if(results.begin(), results.end(),
                [&](const DynamicRangeResult& r){ return r.filename == current_curve.filename && r.channel == current_curve.channel; });

            if (it != results.end()) {
                if (base_geom_12db && it->dr_values_ev.count(12.0)) {
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << it->dr_values_ev.at(12.0) << "EV";
                    auto [px, py, angle] = *base_geom_12db;
                    DrawThresholdIntersection(cr, ss.str(), current_curve.channel, px, py, angle, iso_index, i, group_size);
                }
                if (base_geom_0db && it->dr_values_ev.count(0.0)) {
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << it->dr_values_ev.at(0.0) << "EV";
                    auto [px, py, angle] = *base_geom_0db;
                    DrawThresholdIntersection(cr, ss.str(), current_curve.channel, px, py, angle, iso_index, i, group_size);
                }
            }
        }
        iso_index++;
    }
}