// File: src/core/graphics/Drawing.cpp
/**
 * @file core/graphics/Drawing.cpp
 * @brief Implements the low-level Cairo drawing functions.
 */
#include "Drawing.hpp"
#include "../math/Math.hpp"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <filesystem>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace fs = std::filesystem;

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

} // end of anonymous namespace

void DrawPlotBase(
    cairo_t* cr,
    const std::string& title,
    const std::map<std::string, double>& bounds,
    const std::string& command_text,
    const std::vector<double>& snr_thresholds)
{
    const int margin_left = 180, margin_bottom = 120, margin_top = 100, margin_right = 100;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;
    const int plot_area_height = PLOT_HEIGHT - margin_top - margin_bottom;

    auto map_coords = [&](double ev, double db) {
        double px = margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
        double py = (PLOT_HEIGHT - margin_bottom) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
        return std::make_pair(px, py);
    };

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
    cairo_set_line_width(cr, 1.0);
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        auto [p1x, p1y] = map_coords(ev, bounds.at("min_db"));
        auto [p2x, p2y] = map_coords(ev, bounds.at("max_db"));
        cairo_move_to(cr, p1x, p1y); cairo_line_to(cr, p2x, p2y); cairo_stroke(cr);
    }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), db);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), db);
        cairo_move_to(cr, p1x, p1y); cairo_line_to(cr, p2x, p2y); cairo_stroke(cr);
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 3.0);
    cairo_rectangle(cr, margin_left, margin_top, plot_area_width, plot_area_height);
    cairo_stroke(cr);
    
    // Draw a line for each provided SNR threshold
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
    
    cairo_set_font_size(cr, 16.0);
    cairo_text_extents_t extents;
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) { 
        std::string ev_str = std::to_string((int)ev);
        cairo_text_extents(cr, ev_str.c_str(), &extents);
        auto [px, py] = map_coords(ev, bounds.at("min_db"));
        cairo_move_to(cr, px - extents.width / 2, PLOT_HEIGHT - margin_bottom + 25); 
        cairo_show_text(cr, ev_str.c_str());
    }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) { 
        std::string db_str = std::to_string((int)db);
        cairo_text_extents(cr, db_str.c_str(), &extents);
        auto [px, py] = map_coords(bounds.at("min_ev"), db);
        cairo_move_to(cr, margin_left - extents.width - 15, py + extents.height / 2); 
        cairo_show_text(cr, db_str.c_str());
    }

    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24.0);
    cairo_text_extents(cr, title.c_str(), &extents);
    cairo_move_to(cr, PLOT_WIDTH / 2 - extents.width / 2, margin_top - 40);
    cairo_show_text(cr, title.c_str());

    cairo_set_font_size(cr, 20.0);
    std::string x_label = "RAW exposure (EV)";
    cairo_text_extents(cr, x_label.c_str(), &extents);
    cairo_move_to(cr, PLOT_WIDTH / 2 - extents.width / 2, PLOT_HEIGHT - margin_bottom + 70);
    cairo_show_text(cr, x_label.c_str());
    
    std::string y_label = "SNR (dB)";
    cairo_text_extents(cr, y_label.c_str(), &extents);
    cairo_save(cr);
    cairo_move_to(cr, margin_left / 2.0 - extents.height / 2.0, PLOT_HEIGHT / 2.0 + extents.width / 2.0);
    cairo_rotate(cr, -M_PI / 2.0);
    cairo_show_text(cr, y_label.c_str());
    cairo_restore(cr);

    if (!command_text.empty()) {
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
        cairo_text_extents_t cmd_extents;
        cairo_text_extents(cr, command_text.c_str(), &cmd_extents);
        cairo_move_to(cr, PLOT_WIDTH - margin_right - cmd_extents.width - 10, PLOT_HEIGHT - 20);
        cairo_show_text(cr, command_text.c_str());
    }
}

void OldDrawCurvesAndData(
    cairo_t* cr,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds)
{
    const int margin_left = 180, margin_bottom = 120, margin_top = 100, margin_right = 100;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;
    const int plot_area_height = PLOT_HEIGHT - margin_top - margin_bottom;

    auto map_coords = [&](double ev, double db) {
        double px = margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
        double py = (PLOT_HEIGHT - margin_bottom) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
        return std::make_pair(px, py);
    };

    // Using a boolean to alternate is simpler and more robust.
    bool draw_above_12db = true;
    bool draw_above_0db = true;

    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        
        auto min_max_ev_it = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        double local_min_ev = *(min_max_ev_it.first);
        double local_max_ev = *(min_max_ev_it.second);
        
        cairo_set_source_rgb(cr, 200.0/255.0, 0.0, 0.0);
        cairo_set_line_width(cr, 2.0);
        double snr_poly_start = 0.0;
        for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
            snr_poly_start += curve.poly_coeffs.at<double>(j) * std::pow(curve.signal_ev.front(), curve.poly_coeffs.rows - 1 - j);
        }
        auto [start_x, start_y] = map_coords(curve.signal_ev.front(), snr_poly_start);
        cairo_move_to(cr, start_x, start_y);
        for (double ev = curve.signal_ev.front(); ev <= curve.signal_ev.back(); ev += 0.05) {
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
                snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j);
            }
            auto [px, py] = map_coords(ev, snr_poly);
            cairo_line_to(cr, px, py);
        }
        cairo_stroke(cr);

        cairo_set_source_rgb(cr, 0.0, 0.0, 200.0/255.0);
        for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
            auto [px, py] = map_coords(curve.signal_ev[j], curve.snr_db[j]);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }

        // Use the new plot_label field for the text on the curve
        std::string label = curve.plot_label;
        
        auto [label_x, label_y] = map_coords(curve.signal_ev.back(), curve.snr_db.back());
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14.0);
        cairo_set_source_rgb(cr, 200.0/255.0, 0.0, 0.0);
        cairo_move_to(cr, label_x - 40, label_y - 30);
        cairo_show_text(cr, label.c_str());

        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);

        auto ev12 = FindIntersectionEV(curve.poly_coeffs, 12.0, local_min_ev, local_max_ev);
        if (ev12) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev12 << "EV";
            auto [px, py] = map_coords(*ev12, 12.0);
            
            double offset_x = draw_above_12db ? 25.0 : 15.0;
            double offset_y = draw_above_12db ? -10.0 : 15.0;
            
            cairo_move_to(cr, px + offset_x, py + offset_y);
            cairo_show_text(cr, ss.str().c_str());
            draw_above_12db = !draw_above_12db; // Alternate for the next curve
        }

        auto ev0 = FindIntersectionEV(curve.poly_coeffs, 0.0, local_min_ev, local_max_ev);
        if (ev0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev0 << "EV";
            auto [px, py] = map_coords(*ev0, 0.0);
            
            double offset_x = draw_above_0db ? 20.0 : 15.0;
            double offset_y = draw_above_0db ? -10.0 : 15.0;

            cairo_move_to(cr, px + offset_x, py + offset_y);
            cairo_show_text(cr, ss.str().c_str());
            draw_above_0db = !draw_above_0db; // Alternate for the next curve
        }
    }
}

void DrawCurvesAndData(
    cairo_t* cr,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds)
{
    const int margin_left = 180, margin_bottom = 120, margin_top = 100, margin_right = 100;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;
    const int plot_area_height = PLOT_HEIGHT - margin_top - margin_bottom;
    auto map_coords = [&](double ev, double db) {
        double px = margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
        double py = (PLOT_HEIGHT - margin_bottom) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
        return std::make_pair(px, py);
    };
    // Using a boolean to alternate is simpler and more robust.
    bool draw_above_12db = true;
    bool draw_above_0db = true;
    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        auto min_max_ev_it = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        double local_min_ev = *(min_max_ev_it.first);
        double local_max_ev = *(min_max_ev_it.second);
        cairo_set_source_rgb(cr, 200.0/255.0, 0.0, 0.0);
        cairo_set_line_width(cr, 2.0);
        double snr_poly_start = 0.0;
        for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
            snr_poly_start += curve.poly_coeffs.at<double>(j) * std::pow(curve.signal_ev.front(), curve.poly_coeffs.rows - 1 - j);
        }
        auto [start_x, start_y] = map_coords(curve.signal_ev.front(), snr_poly_start);
        cairo_move_to(cr, start_x, start_y);
        // Evaluar el polinomio solo en el rango de los datos reales
        for (double ev = local_min_ev; ev <= local_max_ev; ev += 0.05) {
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
                snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j);
            }
            auto [px, py] = map_coords(ev, snr_poly);
            cairo_line_to(cr, px, py);
        }
        cairo_stroke(cr);
        cairo_set_source_rgb(cr, 0.0, 0.0, 200.0/255.0);
        for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
            auto [px, py] = map_coords(curve.signal_ev[j], curve.snr_db[j]);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }
        // Use the new plot_label field for the text on the curve
        std::string label = curve.plot_label;
        auto [label_x, label_y] = map_coords(curve.signal_ev.back(), curve.snr_db.back());
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14.0);
        cairo_set_source_rgb(cr, 200.0/255.0, 0.0, 0.0);
        cairo_move_to(cr, label_x - 40, label_y - 30);
        cairo_show_text(cr, label.c_str());
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        auto ev12 = FindIntersectionEV(curve.poly_coeffs, 12.0, local_min_ev, local_max_ev);
        if (ev12) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev12 << "EV";
            auto [px, py] = map_coords(*ev12, 12.0);
            double offset_x = draw_above_12db ? 25.0 : 15.0;
            double offset_y = draw_above_12db ? -10.0 : 15.0;
            cairo_move_to(cr, px + offset_x, py + offset_y);
            cairo_show_text(cr, ss.str().c_str());
            draw_above_12db = !draw_above_12db; // Alternate for the next curve
        }
        auto ev0 = FindIntersectionEV(curve.poly_coeffs, 0.0, local_min_ev, local_max_ev);
        if (ev0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev0 << "EV";
            auto [px, py] = map_coords(*ev0, 0.0);
            double offset_x = draw_above_0db ? 20.0 : 15.0;
            double offset_y = draw_above_0db ? -10.0 : 15.0;
            cairo_move_to(cr, px + offset_x, py + offset_y);
            cairo_show_text(cr, ss.str().c_str());
            draw_above_0db = !draw_above_0db; // Alternate for the next curve
        }
    }
}