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

namespace { // Namespace anónimo para funciones auxiliares internas a este fichero

void DrawDashedLine(cv::Mat& img, cv::Point p1, cv::Point p2, const cv::Scalar& color, int thickness, int dash_length = 20);
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);

void DrawDashedLine(cv::Mat& img, cv::Point p1, cv::Point p2, const cv::Scalar& color, int thickness, int dash_length) {
    double dist = cv::norm(p1 - p2);
    if (dist < 1e-6) return;
    double dx = (p2.x - p1.x) / dist;
    double dy = (p2.y - p1.y) / dist;
    for (double i = 0; i < dist; i += dash_length * 2) {
        cv::Point start = {static_cast<int>(p1.x + i * dx), static_cast<int>(p1.y + i * dy)};
        cv::Point end = {static_cast<int>(p1.x + std::min(i + dash_length, dist) * dx), static_cast<int>(p1.y + std::min(i + dash_length, dist) * dy)};
        cv::line(img, start, end, color, thickness, cv::LINE_AA);
    }
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
    cv::Mat& plot_img,
    const std::string& title,
    const std::map<std::string, double>& bounds)
{
    const int margin_left = 240, margin_bottom = 200, margin_top = 240, margin_right = 600;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;
    const int plot_area_height = PLOT_HEIGHT - margin_top - margin_bottom;

    auto map_coords = [&](double ev, double db) {
        int px = static_cast<int>(margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width);
        int py = static_cast<int>((PLOT_HEIGHT - margin_bottom) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height);
        return cv::Point(px, py);
    };

    cv::rectangle(plot_img, {0,0}, {PLOT_WIDTH, PLOT_HEIGHT}, cv::Scalar(255,255,255), -1);
    
    // --- Comienza el orden de dibujado corregido ---

    // 1. Rejilla
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        cv::line(plot_img, map_coords(ev, bounds.at("min_db")), map_coords(ev, bounds.at("max_db")), {220,220,220}, 2);
    }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        cv::line(plot_img, map_coords(bounds.at("min_ev"), db), map_coords(bounds.at("max_ev"), db), {220,220,220}, 2);
    }

    // 2. Borde principal del área de trazado
    cv::Rect plot_rect(margin_left, margin_top, plot_area_width, plot_area_height);
    cv::rectangle(plot_img, plot_rect, {0,0,0}, 3);
    
    // 3. Líneas de referencia (ahora se dibujan después de la rejilla y el borde para asegurar visibilidad)
    DrawDashedLine(plot_img, map_coords(bounds.at("min_ev"), 12.0), map_coords(bounds.at("max_ev"), 12.0), {0,0,0}, 2);
    cv::putText(plot_img, "Photographic DR (SNR > 12dB)", {map_coords(bounds.at("min_ev"), 12.0).x + 40, map_coords(bounds.at("min_ev"), 12.0).y - 20}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);
    
    DrawDashedLine(plot_img, map_coords(bounds.at("min_ev"), 0.0), map_coords(bounds.at("max_ev"), 0.0), {0,0,0}, 2);
    cv::putText(plot_img, "Engineering DR (SNR > 0dB)", {map_coords(bounds.at("min_ev"), 0.0).x + 40, map_coords(bounds.at("min_ev"), 0.0).y + 40}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);

    // 4. Etiquetas de los ejes (números)
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) { cv::putText(plot_img, std::to_string((int)ev), {map_coords(ev, bounds.at("min_db")).x-15, PLOT_HEIGHT-margin_bottom+45}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2); }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) { cv::putText(plot_img, std::to_string((int)db), {margin_left-70, map_coords(bounds.at("min_ev"), db).y+10}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2); }
    
    // 5. Títulos
    cv::putText(plot_img, "RAW Exposure (EV)", {PLOT_WIDTH/2-150, PLOT_HEIGHT-50}, cv::FONT_HERSHEY_SIMPLEX, 1.2, {0,0,0}, 3);
    cv::putText(plot_img, title, {PLOT_WIDTH/2-300, margin_top-60}, cv::FONT_HERSHEY_SIMPLEX, 2.0, {0,0,0}, 3);

    // 6. Etiqueta del eje Y
    std::string y_label_text = "SNR (dB)";
    cv::Size text_size = cv::getTextSize(y_label_text, cv::FONT_HERSHEY_SIMPLEX, 1.2, 3, nullptr);
    cv::Point text_origin(60, margin_top + (plot_area_height + text_size.width) / 2);
    cv::Mat text_canvas = cv::Mat::zeros(text_size.height + 20, text_size.width + 20, CV_8UC3);
    cv::putText(text_canvas, y_label_text, cv::Point(10, text_size.height + 10), cv::FONT_HERSHEY_SIMPLEX, 1.2, {255,255,255}, 3);
    cv::rotate(text_canvas, text_canvas, cv::ROTATE_90_COUNTERCLOCKWISE);
    cv::Mat text_roi = plot_img(cv::Rect(text_origin.x, text_origin.y - text_canvas.rows, text_canvas.cols, text_canvas.rows));
    text_roi.setTo(cv::Scalar(0,0,0), text_canvas);
}

void DrawCurvesAndData(
    cv::Mat& plot_img,
    const std::vector<CurveData>& curves,
    const std::map<std::string, double>& bounds)
{
    const int margin_left = 240, margin_top = 240, margin_right = 600;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;

    auto map_coords = [&](double ev, double db) {
        int px = static_cast<int>(margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width);
        int py = static_cast<int>((PLOT_HEIGHT - 200) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * (PLOT_HEIGHT - margin_top - 200));
        return cv::Point(px, py);
    };

    for (const auto& curve : curves) {
        if (curve.signal_ev.empty()) continue;
        
        cv::Scalar curve_line_color(0, 0, 200); // Color de línea para las curvas (Rojo en BGR)
        cv::Scalar curve_point_color(200, 0, 0); // Color de puntos (Azul en BGR)

        auto min_max_ev_it = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        double local_min_ev = *(min_max_ev_it.first);
        double local_max_ev = *(min_max_ev_it.second);

        std::vector<cv::Point> poly_points;
        for (double ev = local_min_ev; ev <= local_max_ev; ev += 0.05) {
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) { snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j); }
            poly_points.push_back(map_coords(ev, snr_poly));
        }
        cv::polylines(plot_img, poly_points, false, curve_line_color, 2, cv::LINE_AA); // Grosor de línea ajustado a 2

        for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
            cv::circle(plot_img, map_coords(curve.signal_ev[j], curve.snr_db[j]), 4, curve_point_color, -1, cv::LINE_AA); // Radio de puntos ajustado a 4
        }

        std::string label = fs::path(curve.name).stem().string();
        cv::Point label_pos_end = map_coords(curve.signal_ev.back(), curve.snr_db.back());
        cv::putText(plot_img, label, {label_pos_end.x - 40, label_pos_end.y - 30}, cv::FONT_HERSHEY_SIMPLEX, 1.0, curve_line_color, 2, cv::LINE_AA);

        auto ev12 = FindIntersectionEV(curve.poly_coeffs, 12.0, local_min_ev, local_max_ev);
        if (ev12) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev12 << "EV";
            cv::Point p = map_coords(*ev12, 12.0);
            cv::putText(plot_img, ss.str(), {p.x + 15, p.y - 15}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {0,0,0}, 2);
        }
        auto ev0 = FindIntersectionEV(curve.poly_coeffs, 0.0, local_min_ev, local_max_ev);
        if (ev0) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << *ev0 << "EV";
            cv::Point p = map_coords(*ev0, 0.0);
            cv::putText(plot_img, ss.str(), {p.x + 15, p.y - 15}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {0,0,0}, 2);
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
    cv::Mat plot_img(PLOT_HEIGHT, PLOT_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));
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
    DrawPlotBase(plot_img, "SNR Curve - " + image_title, bounds); // Ya no se pasa 'show_stripe_labels'
    std::vector<CurveData> single_curve_vec = {{image_title, signal_ev, snr_db, poly_coeffs}};
    DrawCurvesAndData(plot_img, single_curve_vec, bounds); // Ya no se pasa 'is_summary'
    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Plot saved to: " << output_filename << std::endl;
}

void GenerateSummaryPlot(
    const std::string& output_filename,
    const std::vector<CurveData>& all_curves)
{
    cv::Mat plot_img(PLOT_HEIGHT, PLOT_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));
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
    DrawPlotBase(plot_img, "SNR Curves - Olympus OM-1", bounds); // Ya no se pasa 'show_stripe_labels'
    DrawCurvesAndData(plot_img, all_curves, bounds); // Ya no se pasa 'is_summary'
    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Summary Plot saved to: " << output_filename << std::endl;
}