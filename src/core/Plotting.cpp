// Fichero: core/Plotting.cpp
#include "Plotting.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <optional>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace { // Namespace anónimo para funciones auxiliares

void DrawDashedLine(wxGraphicsContext* gc, double x1, double y1, double x2, double y2, const wxPen& pen, int dash_length = 20);
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);

void DrawDashedLine(wxGraphicsContext* gc, double x1, double y1, double x2, double y2, const wxPen& pen, int dash_length) {
    gc->SetPen(pen);
    wxGraphicsPath path = gc->CreatePath();
    path.MoveToPoint(x1, y1);
    path.AddLineToPoint(x2, y2);
    gc->StrokePath(path);
}

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

void DrawPlotBase(
    wxGraphicsContext* gc,
    const std::string& title,
    const std::map<std::string, double>& bounds)
{
    const int margin_left = 180, margin_bottom = 120, margin_top = 100, margin_right = 100;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;
    const int plot_area_height = PLOT_HEIGHT - margin_top - margin_bottom;

    auto map_coords = [&](double ev, double db) {
        double px = margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
        double py = (PLOT_HEIGHT - margin_bottom) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
        return wxPoint2DDouble(px, py);
    };

    // Fondo blanco
    gc->SetBrush(*wxWHITE_BRUSH);
    gc->DrawRectangle(0, 0, PLOT_WIDTH, PLOT_HEIGHT);

    // Rejilla
    wxPen light_grey_pen(*wxLIGHT_GREY, 1);
    gc->SetPen(light_grey_pen);
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        gc->StrokeLine(map_coords(ev, bounds.at("min_db")).m_x, map_coords(ev, bounds.at("min_db")).m_y, map_coords(ev, bounds.at("max_db")).m_x, map_coords(ev, bounds.at("max_db")).m_y);
    }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        gc->StrokeLine(map_coords(bounds.at("min_ev"), db).m_x, map_coords(bounds.at("min_ev"), db).m_y, map_coords(bounds.at("max_ev"), db).m_x, map_coords(bounds.at("max_ev"), db).m_y);
    }

    // Borde principal del área de trazado
    wxPen black_pen(*wxBLACK, 3);
    gc->SetPen(black_pen);
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(margin_left, margin_top, plot_area_width, plot_area_height);
    
    // Líneas de referencia
    wxPen dashed_pen(*wxBLACK, 2, wxPENSTYLE_DOT_DASH);
    DrawDashedLine(gc, map_coords(bounds.at("min_ev"), 12.0).m_x, map_coords(bounds.at("min_ev"), 12.0).m_y, map_coords(bounds.at("max_ev"), 12.0).m_x, map_coords(bounds.at("max_ev"), 12.0).m_y, dashed_pen);
    gc->SetFont(*wxNORMAL_FONT, *wxBLACK);
    gc->DrawText("Photographic DR (SNR > 12dB)", map_coords(bounds.at("min_ev"), 12.0).m_x + 20, map_coords(bounds.at("min_ev"), 12.0).m_y - 20);
    
    DrawDashedLine(gc, map_coords(bounds.at("min_ev"), 0.0).m_x, map_coords(bounds.at("min_ev"), 0.0).m_y, map_coords(bounds.at("max_ev"), 0.0).m_x, map_coords(bounds.at("max_ev"), 0.0).m_y, dashed_pen);
    gc->DrawText("Engineering DR (SNR > 0dB)", map_coords(bounds.at("min_ev"), 0.0).m_x + 20, map_coords(bounds.at("min_ev"), 0.0).m_y - 20);

    // Etiquetas de los ejes
    gc->SetFont(*wxNORMAL_FONT, *wxBLACK);
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) { 
        double text_width, text_height;
        std::string ev_str = std::to_string((int)ev);
        gc->GetTextExtent(ev_str, &text_width, &text_height);
        gc->DrawText(ev_str, map_coords(ev, bounds.at("min_db")).m_x - (text_width / 2), PLOT_HEIGHT - margin_bottom + 20); 
    }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) { 
        double text_width, text_height;
        std::string db_str = std::to_string((int)db);
        gc->GetTextExtent(db_str, &text_width, &text_height);
        gc->DrawText(db_str, margin_left - text_width - 15, map_coords(bounds.at("min_ev"), db).m_y - (text_height / 2)); 
    }

    // Títulos
    gc->SetFont(wxFont(24, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxBLACK);
    gc->DrawText(title, PLOT_WIDTH / 2 - 300, margin_top - 60);

    // Etiquetas de los ejes mejoradas
    gc->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxBLACK);
    
    double text_width_x, text_height_x;
    gc->GetTextExtent("RAW exposure (EV)", &text_width_x, &text_height_x);
    gc->DrawText("RAW exposure (EV)", PLOT_WIDTH / 2 - (text_width_x / 2), PLOT_HEIGHT - margin_bottom + 60);
    
    wxGraphicsMatrix matrix = gc->CreateMatrix();
    matrix.Rotate(-M_PI / 2.0);
    double snr_text_width, snr_text_height;
    gc->GetTextExtent("SNR (dB)", &snr_text_width, &snr_text_height);
    matrix.Translate(-(PLOT_HEIGHT / 2.0 + snr_text_width / 2.0), margin_left / 2.0);
    gc->SetTransform(matrix);
    gc->DrawText("SNR (dB)", 0, 0);
    gc->SetTransform(gc->CreateMatrix());
}


void DrawCurvesAndData(
    wxGraphicsContext* gc,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds)
{
    const int margin_left = 180, margin_top = 100, margin_right = 100;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;
    const int plot_area_height = PLOT_HEIGHT - margin_top - 120;

    auto map_coords = [&](double ev, double db) {
        double px = margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width;
        double py = (PLOT_HEIGHT - 120) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height;
        return wxPoint2DDouble(px, py);
    };

    bool draw_above_12db = true;
    bool draw_above_0db = true;

    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        
        wxPen curve_line_pen(wxColour(200, 0, 0), 2);
        wxBrush curve_point_brush(wxColour(0, 0, 200));

        auto min_max_ev_it = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        double local_min_ev = *(min_max_ev_it.first);
        double local_max_ev = *(min_max_ev_it.second);

        // Curva del polinomio
        wxGraphicsPath poly_path = gc->CreatePath();
        double snr_poly_start = 0.0;
        for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
            snr_poly_start += curve.poly_coeffs.at<double>(j) * std::pow(local_min_ev, curve.poly_coeffs.rows - 1 - j);
        }
        poly_path.MoveToPoint(map_coords(local_min_ev, snr_poly_start));
        
        for (double ev = local_min_ev; ev <= local_max_ev; ev += 0.05) {
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) {
                snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j);
            }
            poly_path.AddLineToPoint(map_coords(ev, snr_poly));
        }
        gc->SetPen(curve_line_pen);
        gc->StrokePath(poly_path);

        // Puntos de datos
        gc->SetBrush(curve_point_brush);
        gc->SetPen(*wxTRANSPARENT_PEN); 
        for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
            wxPoint2DDouble point = map_coords(curve.signal_ev[j], curve.snr_db[j]);
            gc->DrawEllipse(point.m_x - 2, point.m_y - 2, 4, 4);
        }

        // Etiquetas
        std::string label = fs::path(curve.name).stem().string();
        wxPoint2DDouble label_pos_end = map_coords(curve.signal_ev.back(), curve.snr_db.back());
        gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), wxColour(200, 0, 0));
        gc->DrawText(label, label_pos_end.m_x - 40, label_pos_end.m_y - 30);

        auto ev12 = FindIntersectionEV(curve.poly_coeffs, 12.0, local_min_ev, local_max_ev);
        if (ev12) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev12 << "EV";
            wxPoint2DDouble p = map_coords(*ev12, 12.0);
            gc->SetFont(*wxNORMAL_FONT, *wxBLACK);
            
            // CORRECCIÓN: Ajuste de posición de las etiquetas EV (12dB)
            if (draw_above_12db) {
                gc->DrawText(ss.str(), p.m_x + 15, p.m_y - 15);
            } else {
                gc->DrawText(ss.str(), p.m_x + 15, p.m_y + 5);
            }
            draw_above_12db = !draw_above_12db;
        }

        auto ev0 = FindIntersectionEV(curve.poly_coeffs, 0.0, local_min_ev, local_max_ev);
        if (ev0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev0 << "EV";
            wxPoint2DDouble p = map_coords(*ev0, 0.0);
            gc->SetFont(*wxNORMAL_FONT, *wxBLACK);

            // CORRECCIÓN: Ajuste de posición de las etiquetas EV (0dB)
            if (draw_above_0db) {
                gc->DrawText(ss.str(), p.m_x + 25, p.m_y - 15);
            } else {
                gc->DrawText(ss.str(), p.m_x + 15, p.m_y + 5);
            }
            draw_above_0db = !draw_above_0db;
        }
    }
}

} // fin del namespace anónimo

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
    wxBitmap bmp(PLOT_WIDTH, PLOT_HEIGHT);
    wxGraphicsContext* gc = wxGraphicsContext::Create(bmp);
    if (!gc) {
        std::cerr << "[ERROR] Failed to create graphics context." << std::endl;
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

    DrawPlotBase(gc, "SNR Curve - " + image_title, bounds);
    std::vector<CurveData> single_curve_vec = {{image_title, signal_ev, snr_db, poly_coeffs}};
    DrawCurvesAndData(gc, single_curve_vec, bounds);
    
    bmp.SaveFile(output_filename, wxBITMAP_TYPE_PNG);
    delete gc;
    std::cout << "[INFO] Plot saved to: " << output_filename << std::endl;
}

void GenerateSummaryPlot(
    const std::string& output_filename,
    const std::vector<CurveData>& all_curves)
{
    wxBitmap bmp(PLOT_WIDTH, PLOT_HEIGHT);
    wxGraphicsContext* gc = wxGraphicsContext::Create(bmp);
    if (!gc) {
        std::cerr << "[ERROR] Failed to create graphics context." << std::endl;
        return;
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
        delete gc;
        return;
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

    DrawPlotBase(gc, "SNR Curves - Olympus OM-1", bounds);
    DrawCurvesAndData(gc, all_curves, bounds);

    bmp.SaveFile(output_filename, wxBITMAP_TYPE_PNG);
    delete gc;
    std::cout << "[INFO] Summary Plot saved to: " << output_filename << std::endl;
}