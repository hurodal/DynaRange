// Fichero: core/Plotting.cpp
#include "Plotting.hpp"
#include <cairo/cairo.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <optional>
#include <iomanip>
#include <sstream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace fs = std::filesystem;

namespace { // Namespace anónimo para funciones auxiliares

// Declaraciones adelantadas
void DrawPlotBase(cairo_t* cr, const std::string& title, const std::map<std::string, double>& bounds);
void DrawCurvesAndData(cairo_t* cr, const std::vector<CurveData>& curves, const std::map<std::string, double>& bounds);
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);

// Dibuja una línea discontinua
void DrawDashedLine(cairo_t* cr, double x1, double y1, double x2, double y2, double dash_length = 20.0) {
    double dashes[] = {dash_length, dash_length};
    cairo_save(cr);
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
    cairo_restore(cr);
}

// Encuentra la intersección de la curva polinómica con un valor de SNR objetivo
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev) {
    if (coeffs.rows < 3) return std::nullopt;
    double c2 = coeffs.at<double>(0), c1 = coeffs.at<double>(1), c0 = coeffs.at<double>(2);
    double a = c2, b = c1, c = c0 - target_snr_db;
    double discriminant = b * b - 4 * a * c;
    if (discriminant < 0) return std::nullopt;
    double sqrt_d = sqrt(discriminant);
    double ev1 = (-b + sqrt_d) / (2 * a);
    double ev2 = (-b - sqrt_d) / (2 * a);
    if (ev1 >= min_ev && ev1 <= max_ev) return ev1;
    if (ev2 >= min_ev && ev2 <= max_ev) return ev2;
    return std::nullopt;
}

// Dibuja la base de la gráfica (ejes, rejilla, títulos)
void DrawPlotBase(
    cairo_t* cr,
    const std::string& title,
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

    // Fondo blanco
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_fill(cr);

    // Rejilla
    cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
    cairo_set_line_width(cr, 1.0);
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        auto [p1x, p1y] = map_coords(ev, bounds.at("min_db"));
        auto [p2x, p2y] = map_coords(ev, bounds.at("max_db"));
        cairo_move_to(cr, p1x, p1y);
        cairo_line_to(cr, p2x, p2y);
        cairo_stroke(cr);
    }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), db);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), db);
        cairo_move_to(cr, p1x, p1y);
        cairo_line_to(cr, p2x, p2y);
        cairo_stroke(cr);
    }

    // Borde principal del área de trazado
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 3.0);
    cairo_rectangle(cr, margin_left, margin_top, plot_area_width, plot_area_height);
    cairo_stroke(cr);
    
    // Líneas de referencia
    cairo_set_line_width(cr, 2.0);
    auto [p12_1x, p12_1y] = map_coords(bounds.at("min_ev"), 12.0);
    auto [p12_2x, p12_2y] = map_coords(bounds.at("max_ev"), 12.0);
    DrawDashedLine(cr, p12_1x, p12_1y, p12_2x, p12_2y);
    
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 16.0);
    cairo_move_to(cr, p12_1x + 20, p12_1y - 10);
    cairo_show_text(cr, "Photographic DR (SNR > 12dB)");

    auto [p0_1x, p0_1y] = map_coords(bounds.at("min_ev"), 0.0);
    auto [p0_2x, p0_2y] = map_coords(bounds.at("max_ev"), 0.0);
    DrawDashedLine(cr, p0_1x, p0_1y, p0_2x, p0_2y);
    cairo_move_to(cr, p0_1x + 20, p0_1y - 10);
    cairo_show_text(cr, "Engineering DR (SNR > 0dB)");

    // Etiquetas de los ejes
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

    // Títulos
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24.0);
    cairo_text_extents(cr, title.c_str(), &extents);
    cairo_move_to(cr, PLOT_WIDTH / 2 - extents.width / 2, margin_top - 40);
    cairo_show_text(cr, title.c_str());

    // Etiquetas de los ejes
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
}

// Dibuja las curvas y los puntos de datos
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

    bool draw_above_12db = true;
    bool draw_above_0db = true;

    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        
        auto min_max_ev_it = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        double local_min_ev = *(min_max_ev_it.first);
        double local_max_ev = *(min_max_ev_it.second);

        // Curva del polinomio
        cairo_set_source_rgb(cr, 200.0/255.0, 0.0, 0.0); // Rojo
        cairo_set_line_width(cr, 2.0);

        double snr_poly_start = 0.0;
        for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
            snr_poly_start += curve.poly_coeffs.at<double>(j) * std::pow(local_min_ev, curve.poly_coeffs.rows - 1 - j);
        }
        auto [start_x, start_y] = map_coords(local_min_ev, snr_poly_start);
        cairo_move_to(cr, start_x, start_y);
        
        for (double ev = local_min_ev; ev <= local_max_ev; ev += 0.05) {
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
                snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j);
            }
            auto [px, py] = map_coords(ev, snr_poly);
            cairo_line_to(cr, px, py);
        }
        cairo_stroke(cr);

        // Puntos de datos
        cairo_set_source_rgb(cr, 0.0, 0.0, 200.0/255.0); // Azul
        for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
            auto [px, py] = map_coords(curve.signal_ev[j], curve.snr_db[j]);
            cairo_arc(cr, px, py, 2.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }

        // Etiquetas de las curvas
        std::string label = fs::path(curve.name).stem().string();
        auto [label_x, label_y] = map_coords(curve.signal_ev.back(), curve.snr_db.back());
        
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 14.0);
        cairo_set_source_rgb(cr, 200.0/255.0, 0.0, 0.0); // Rojo
        cairo_move_to(cr, label_x - 40, label_y - 30);
        cairo_show_text(cr, label.c_str());

        // *****
        // CORRECCIÓN DEFINITIVA: Lógica de las etiquetas EV restaurada
        // para coincidir con el comportamiento de la versión funcional original.
        // *****
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Negro
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 16.0);
        
        auto ev12 = FindIntersectionEV(curve.poly_coeffs, 12.0, local_min_ev, local_max_ev);
        if (ev12) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev12 << "EV";
            auto [px, py] = map_coords(*ev12, 12.0);
            
            double offset_y = draw_above_12db ? -15.0 : 5.0;
            cairo_move_to(cr, px + 15, py + offset_y);
            cairo_show_text(cr, ss.str().c_str());
            draw_above_12db = !draw_above_12db;
        }

        auto ev0 = FindIntersectionEV(curve.poly_coeffs, 0.0, local_min_ev, local_max_ev);
        if (ev0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev0 << "EV";
            auto [px, py] = map_coords(*ev0, 0.0);
            
            double offset_y = draw_above_0db ? -15.0 : 5.0;
            cairo_move_to(cr, px + 25, py + offset_y);
            cairo_show_text(cr, ss.str().c_str());
            draw_above_0db = !draw_above_0db;
        }
    }
}

} // fin del namespace anónimo


// --- FUNCIONES PÚBLICAS ---

void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs)
{
    if (signal_ev.size() < 2) {
        std::cout << "[WARNING] Skipping plot for \"" << image_title << "\" due to insufficient data points (" << signal_ev.size() << ")." << std::endl;
        return;
    }
    
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        std::cerr << "[ERROR] Failed to create cairo context." << std::endl;
        cairo_surface_destroy(surface);
        return;
    }

    auto min_max_ev = std::minmax_element(signal_ev.begin(), signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;
    std::map<std::string, double> bounds;
    if (max_ev_data - min_ev_data < 1e-6) {
        bounds["min_ev"] = min_ev_data - 0.5;
        bounds["max_ev"] = max_ev_data + 0.5;
    } else {
        bounds["min_ev"] = floor(min_ev_data) - 1.0;
        bounds["max_ev"] = ceil(max_ev_data) + 1.0;
    }
    bounds["min_db"] = -15.0;
    bounds["max_db"] = 25.0;

    DrawPlotBase(cr, "SNR Curve - " + image_title, bounds);
    std::vector<CurveData> single_curve_vec = {{image_title, "", signal_ev, snr_db, poly_coeffs}};
    DrawCurvesAndData(cr, single_curve_vec, bounds);
    
    cairo_surface_write_to_png(surface, output_filename.c_str());
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    std::cout << "[INFO] Plot saved to2: " << output_filename << std::endl;
}

std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_dir,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves)
{
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        std::cerr << "[ERROR] Failed to create cairo context." << std::endl;
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

    double min_ev_global = 1e6, max_ev_global = -1e6;
    bool has_data = false;
    for (const auto& curve : all_curves) {
        if (!curve.signal_ev.empty()) {
            has_data = true;
            min_ev_global = std::min(min_ev_global, *std::min_element(curve.signal_ev.begin(), curve.signal_ev.end()));
            max_ev_global = std::max(max_ev_global, *std::max_element(curve.signal_ev.begin(), curve.signal_ev.end()));
        }
    }
    if (!has_data) {
        std::cout << "[WARNING] Skipping summary plot due to no data points." << std::endl;
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

    std::map<std::string, double> bounds;
    if (max_ev_global - min_ev_global < 1e-6) {
        bounds["min_ev"] = min_ev_global - 0.5;
        bounds["max_ev"] = max_ev_global + 0.5;
    } else {
        bounds["min_ev"] = floor(min_ev_global) - 1.0;
        bounds["max_ev"] = ceil(max_ev_global) + 1.0;
    }
    bounds["min_db"] = -15.0;
    bounds["max_db"] = 25.0;

    std::string title = "SNR Curves - Summary";
    std::string filename_suffix = "";
    if (!camera_name.empty()) {
        title += " (" + camera_name + ")";
        
        std::string safe_camera_name = camera_name;
        std::replace(safe_camera_name.begin(), safe_camera_name.end(), ' ', '_');
        filename_suffix = "_" + safe_camera_name;
    }

    std::string output_filename = (fs::path(output_dir) / ("DR_summary_plot" + filename_suffix + ".png")).string();

    DrawPlotBase(cr, title, bounds);
    DrawCurvesAndData(cr, all_curves, bounds);

    cairo_surface_write_to_png(surface, output_filename.c_str());
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    std::cout << "[INFO] Summary Plot saved to: " << output_filename << std::endl;
    
    return output_filename;
}