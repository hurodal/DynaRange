// File: src/core/graphics/PlotData.cpp
/**
 * @file src/core/graphics/PlotData.cpp
 * @brief Implements the low-level Cairo drawing functions for the dynamic plot data.
 */
#include "PlotData.hpp"
#include "../math/Math.hpp"
#include "Colour.hpp"
#include "FontManager.hpp"
#include "PlotBase.hpp"
#include <algorithm>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <tuple>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Anonymous namespace for internal helper functions
namespace {

void DrawCurve(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const DynaRange::Graphics::RenderContext& ctx, double alpha)
{
    if (curve.curve_points.empty()) {
        return;
    }
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };

    PlotColors::SetSourceFromChannelWithAlpha(cr, curve.channel, alpha);
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

void DrawDataPoints(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const DynaRange::Graphics::RenderContext& ctx, double alpha)
{
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };

    if (curve.channel == DataSource::AVG) {
        for (const auto& point : curve.points) {
            PlotColors::SetSourceFromChannelWithAlpha(cr, point.channel, alpha);
            auto [px, py] = map_coords(point.ev, point.snr_db);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    } else {
        PlotColors::SetSourceFromChannelWithAlpha(cr, curve.channel, alpha);
        for (const auto& point : curve.points) {
            auto [px, py] = map_coords(point.ev, point.snr_db);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }
}

void DrawCurveLabel(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const DynaRange::Graphics::RenderContext& ctx)
{
    if (curve.points.empty())
        return;
    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };
    std::string label = curve.plot_label;

    // Find the point with the maximum EV to place the label.
    auto max_ev_point_it = std::max_element(curve.points.begin(), curve.points.end(), [](const PointData& a, const PointData& b) { return a.ev < b.ev; });

    auto [label_x, label_y] = map_coords(max_ev_point_it->ev, max_ev_point_it->snr_db);

    const DynaRange::Graphics::FontManager font_manager(ctx);
    PlotColors::cairo_set_source_black(cr);
    font_manager.SetCurveLabelFont(cr);

    // Reduced offsets to bring label closer to the curve's end point.
    cairo_move_to(cr, label_x + 10, label_y - 15);
    cairo_show_text(cr, label.c_str());
}

void DrawThresholdIntersection(cairo_t* cr, const std::string& text_to_draw, DataSource channel, double primary_px, double primary_py, double primary_angle_rad, int iso_index, int channel_index,
    int group_size, const std::map<std::string, double>& bounds, const DynaRange::Graphics::RenderContext& ctx)
{
    // --- 1. Calculate Position Offset relative to the center of the block ---
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
    const DynaRange::Graphics::FontManager font_manager(ctx);
    PlotColors::SetSourceFromChannel(cr, channel);
    font_manager.SetDrValueFont(cr);

    cairo_translate(cr, primary_px, primary_py);
    cairo_rotate(cr, primary_angle_rad);

    cairo_move_to(cr, H_OFFSET_FROM_BASE_LINE, v_offset_from_center);
    cairo_show_text(cr, text_to_draw.c_str());

    cairo_restore(cr);
}

} // end anonymous namespace

void DrawCurvesAndData(cairo_t* cr, const DynaRange::Graphics::RenderContext& ctx, const PlotInfoBox& info_box, const std::vector<CurveData>& curves,

    const std::vector<DynamicRangeResult>& results, const std::map<std::string, double>& bounds, const ProgramOptions& opts)
{
    info_box.Draw(cr, ctx);
    // --- Define the desired drawing order (from back to front) ---
    const std::vector<DataSource> draw_order = { DataSource::AVG, DataSource::G1, DataSource::G2, DataSource::R, DataSource::B };
    // --- Create a new vector of curves sorted by the desired draw order ---
    std::vector<const CurveData*> sorted_curves;
    for (DataSource channel_to_find : draw_order) {
        for (const auto& curve : curves) {
            if (curve.channel == channel_to_find) {
                sorted_curves.push_back(&curve);
            }
        }
    }

    // --- PASS 1: Draw sorted curves and points with new opacity logic ---
    for (size_t i = 0; i < sorted_curves.size(); ++i) {
        const CurveData& curve = *sorted_curves[i];
        if (curve.points.empty())
            continue;

        // The first curve (index 0) is 100% opaque. All subsequent curves have a fixed transparency.
        double alpha = (i == 0) ? 1.0 : (1.0 - PlotColors::OPACITY_DECREMENT_STEP);

        if (opts.plot_details.show_curve) {
            DrawCurve(cr, curve, bounds, ctx, alpha);
        }
        if (opts.plot_details.show_scatters) {
            DrawDataPoints(cr, curve, bounds, ctx, alpha);
        }
    }

    // --- PASS 2: Draw labels (always on top and fully opaque) ---
    if (!opts.plot_details.show_labels) {
        return; // Skip all label drawing if disabled
    }

    std::set<std::string> drawn_iso_labels;
    // Group curves by filename (ISO) to handle them as a block
    std::map<std::string, std::vector<const CurveData*>> curves_by_iso;
    const std::vector<DataSource> canonical_order = { DataSource::R, DataSource::G1, DataSource::G2, DataSource::B, DataSource::AVG };
    for (DataSource ch : canonical_order) {
        for (const auto& c : curves) {
            if (c.channel == ch) {
                curves_by_iso[c.filename].push_back(&c);
            }
        }
    }

    int iso_index = 0;
    for (const auto& pair : curves_by_iso) {
        const auto& iso_curves_group = pair.second;
        if (iso_curves_group.empty())
            continue;

        // 1. Identify the primary curve and draw its main label
        const CurveData& primary_curve = *iso_curves_group[0];
        if (drawn_iso_labels.find(primary_curve.plot_label) == drawn_iso_labels.end()) {
            DrawCurveLabel(cr, primary_curve, bounds, ctx);
            drawn_iso_labels.insert(primary_curve.plot_label);
        }

        // 2. Calculate the "base line" geometry (position and angle) ONCE from the primary curve
        auto calculate_base_geometry = [&](double threshold) -> std::optional<std::tuple<double, double, double>> {
            auto it = std::find_if(results.begin(), results.end(), [&](const DynamicRangeResult& r) { return r.filename == primary_curve.filename && r.channel == primary_curve.channel; });
            if (it != results.end() && it->dr_values_ev.count(threshold)) {
                double dr_value = it->dr_values_ev.at(threshold);
                if (dr_value <= 0)
                    return std::nullopt;

                double ev = -dr_value;
                auto [px, py] = MapToPixelCoords(ev, threshold, bounds, ctx);
                double dEV_dSNR = EvaluatePolynomialDerivative(primary_curve.poly_coeffs, threshold);

                double slope = (std::abs(dEV_dSNR) < 1e-9) ? 1e9 : (1.0 / dEV_dSNR);
                const double plot_w = ctx.base_width - MARGIN_LEFT - MARGIN_RIGHT;
                const double plot_h = ctx.base_height - MARGIN_TOP - MARGIN_BOTTOM;
                const double range_ev = bounds.at("max_ev") - bounds.at("min_ev");
                const double range_db = bounds.at("max_db") - bounds.at("min_db");
                double slope_px = -slope * (plot_h / range_db) / (plot_w / range_ev);
                double angle = std::atan(slope_px);

                return { { px, py, angle } };
            }
            return std::nullopt;
        };
        // Get all unique SNR thresholds from the results to be plotted
        std::set<double> snr_thresholds_to_plot;
        for (const auto& res : results) {
            for (const auto& p : res.dr_values_ev) {
                snr_thresholds_to_plot.insert(p.first);
            }
        }

        // Iterate over all thresholds that have results
        for (double threshold : snr_thresholds_to_plot) {
            auto base_geometry = calculate_base_geometry(threshold);
            if (!base_geometry) {
                continue;
                // Cannot determine geometry if primary curve doesn't have this threshold
            }

            auto [px, py, angle] = *base_geometry;
            int group_size = iso_curves_group.size();

            // Draw all labels in the group for the current threshold
            for (int i = 0; i < group_size; ++i) {
                const CurveData& current_curve = *iso_curves_group[i];
                auto it = std::find_if(results.begin(), results.end(), [&](const DynamicRangeResult& r) { return r.filename == current_curve.filename && r.channel == current_curve.channel; });
                if (it != results.end() && it->dr_values_ev.count(threshold)) {
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << it->dr_values_ev.at(threshold) << "EV";
                    DrawThresholdIntersection(cr, ss.str(), current_curve.channel, px, py, angle, iso_index, i, group_size, bounds, ctx);
                }
            }
        }
        iso_index++;
    }
}