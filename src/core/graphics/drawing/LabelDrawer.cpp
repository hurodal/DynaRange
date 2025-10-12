// File: src/core/graphics/drawing/LabelDrawer.cpp
/**
 * @file LabelDrawer.cpp
 * @brief Implements the drawing logic for plot labels.
 */
#include "LabelDrawer.hpp"
#include "../FontManager.hpp"
#include "../PlotBase.hpp"
#include "../Colour.hpp"
#include "../../math/Math.hpp"
#include <algorithm>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <tuple>

namespace DynaRange::Graphics::Drawing {

void LabelDrawer::Draw(cairo_t* cr, const std::vector<CurveData>& curves, const std::vector<DynamicRangeResult>& results, const std::map<std::string, double>& bounds, const RenderContext& ctx) const
{
    std::set<std::string> drawn_iso_labels;
    std::map<std::string, std::vector<const CurveData*>> curves_by_iso;
    const std::vector<DataSource> canonical_order = { DataSource::R, DataSource::G1, DataSource::G2, DataSource::B, DataSource::AVG };
    for (DataSource ch : canonical_order) {
        for (const auto& c : curves) {
            if (c.channel == ch) {
                curves_by_iso[c.filename].push_back(&c);
            }
        }
    }

    for (const auto& pair : curves_by_iso) {
        const auto& iso_curves_group = pair.second;
        if (iso_curves_group.empty())
            continue;
        
        const CurveData& primary_curve = *iso_curves_group[0];
        if (drawn_iso_labels.find(primary_curve.plot_label) == drawn_iso_labels.end()) {
            DrawCurveLabel(cr, primary_curve, bounds, ctx);
            drawn_iso_labels.insert(primary_curve.plot_label);
        }

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

        std::set<double> snr_thresholds_to_plot;
        for (const auto& res : results) {
            for (const auto& p : res.dr_values_ev) {
                snr_thresholds_to_plot.insert(p.first);
            }
        }

        for (double threshold : snr_thresholds_to_plot) {
            auto base_geometry = calculate_base_geometry(threshold);
            if (!base_geometry) {
                continue;
            }

            auto [px, py, angle] = *base_geometry;
            int group_size = iso_curves_group.size();

            for (size_t i = 0; i < iso_curves_group.size(); ++i) {
                const CurveData& current_curve = *iso_curves_group[i];
                auto it = std::find_if(results.begin(), results.end(), [&](const DynamicRangeResult& r) { return r.filename == current_curve.filename && r.channel == current_curve.channel; });
                if (it != results.end() && it->dr_values_ev.count(threshold)) {
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << it->dr_values_ev.at(threshold) << "EV";
                    DrawThresholdIntersection(cr, ss.str(), current_curve.channel, px, py, angle, i, group_size, ctx);
                }
            }
        }
    }
}

void LabelDrawer::DrawCurveLabel(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const RenderContext& ctx) const
{
    if (curve.points.empty())
        return;

    auto map_coords = [&](double ev, double db) { return MapToPixelCoords(ev, db, bounds, ctx); };
    std::string label = curve.plot_label;

    auto max_ev_point_it = std::max_element(curve.points.begin(), curve.points.end(), [](const PointData& a, const PointData& b) { return a.ev < b.ev; });
    auto [label_x, label_y] = map_coords(max_ev_point_it->ev, max_ev_point_it->snr_db);

    const FontManager font_manager(ctx);
    PlotColors::cairo_set_source_black(cr);
    font_manager.SetCurveLabelFont(cr);
    cairo_move_to(cr, label_x + 10, label_y - 15);
    cairo_show_text(cr, label.c_str());
}

void LabelDrawer::DrawThresholdIntersection(cairo_t* cr, const std::string& text_to_draw, DataSource channel, double primary_px, double primary_py, double primary_angle_rad, int channel_index, int group_size, const RenderContext& ctx) const
{
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

    cairo_save(cr);
    const FontManager font_manager(ctx);
    PlotColors::SetSourceFromChannel(cr, channel);
    font_manager.SetDrValueFont(cr);

    cairo_translate(cr, primary_px, primary_py);
    cairo_rotate(cr, primary_angle_rad);
    cairo_move_to(cr, H_OFFSET_FROM_BASE_LINE, v_offset_from_center);
    cairo_show_text(cr, text_to_draw.c_str());
    cairo_restore(cr);
}

} // namespace DynaRange::Graphics::Drawing