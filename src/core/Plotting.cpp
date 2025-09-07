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

// Declaraciones anticipadas para que las funciones se puedan llamar entre sí
void DrawDashedLine(cv::Mat& img, cv::Point p1, cv::Point p2, const cv::Scalar& color, int thickness, int dash_length = 15); // dash_length más corto
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);

// --- Implementación de las nuevas funciones auxiliares ---

void DrawDashedLine(cv::Mat& img, cv::Point p1, cv::Point p2, const cv::Scalar& color, int thickness, int dash_length) {
    double dist = cv::norm(p1 - p2);
    double dx = (p2.x - p1.x) / dist;
    double dy = (p2.y - p1.y) / dist;

    // Ajuste para asegurar que el final de la línea sea correcto
    for (double i = 0; i < dist; i += dash_length * 2) {
        cv::Point start = {static_cast<int>(p1.x + i * dx), static_cast<int>(p1.y + i * dy)};
        cv::Point end = {static_cast<int>(p1.x + std::min(i + dash_length, dist) * dx), static_cast<int>(p1.y + std::min(i + dash_length, dist) * dy)};
        cv::line(img, start, end, color, thickness, cv::LINE_AA);
    }
}

std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev) {
    if (coeffs.rows < 3) return std::nullopt; // Solo para polinomios de grado 2 (ax^2 + bx + c)

    double c2 = coeffs.at<double>(0); // a
    double c1 = coeffs.at<double>(1); // b
    double c0 = coeffs.at<double>(2); // c
    
    // Resolvemos ax^2 + bx + (c - y) = 0
    double a = c2;
    double b = c1;
    double c = c0 - target_snr_db;

    double discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return std::nullopt;

    double sqrt_d = sqrt(discriminant);
    double ev1 = (-b + sqrt_d) / (2 * a);
    double ev2 = (-b - sqrt_d) / (2 * a);

    // Devolvemos la raíz que esté dentro del rango de datos válido
    // Priorizamos la que esté más a la derecha (mayor EV) si ambas son válidas y tienen sentido en el contexto de la curva SNR.
    // Para una curva SNR típica, la intersección relevante suele ser la de mayor EV.
    if (ev1 >= min_ev && ev1 <= max_ev && ev2 >= min_ev && ev2 <= max_ev) {
        return std::max(ev1, ev2);
    } else if (ev1 >= min_ev && ev1 <= max_ev) {
        return ev1;
    } else if (ev2 >= min_ev && ev2 <= max_ev) {
        return ev2;
    }
    
    return std::nullopt;
}

// --- FUNCIÓN AUXILIAR 1: DIBUJA LA BASE DEL GRÁFICO ---
void DrawPlotBase(
    cv::Mat& plot_img,
    const std::string& title,
    const std::map<std::string, double>& bounds,
    bool show_stripe_labels) // Ahora solo controla si se muestran las etiquetas de las franjas
{
    const int margin_left = 240, margin_bottom = 200, margin_top = 240, margin_right = 600;
    const int plot_area_width = PLOT_WIDTH - margin_left - margin_right;
    const int plot_area_height = PLOT_HEIGHT - margin_top - margin_bottom;

    auto map_coords = [&](double ev, double db) {
        int px = static_cast<int>(margin_left + (ev - bounds.at("min_ev")) / (bounds.at("max_ev") - bounds.at("min_ev")) * plot_area_width);
        int py = static_cast<int>((PLOT_HEIGHT - margin_bottom) - (db - bounds.at("min_db")) / (bounds.at("max_db") - bounds.at("min_db")) * plot_area_height);
        return cv::Point(px, py);
    };

    // Fondo general siempre blanco
    cv::rectangle(plot_img, {0,0}, {PLOT_WIDTH, PLOT_HEIGHT}, cv::Scalar(255,255,255), -1); 
    
    // Dibujar las 5 franjas (3 blancas, 2 celestes) para TODOS los gráficos
    cv::Scalar light_blue(255, 248, 224); // Celeste muy pálido

    // Franja blanca superior (de 17dB a max_db) - "Parches desechados"
    cv::Rect stripe_discarded_top(margin_left, margin_top, plot_area_width, map_coords(0, 17).y - margin_top);
    cv::rectangle(plot_img, stripe_discarded_top, cv::Scalar(255,255,255), -1);
    if (show_stripe_labels) {
        cv::putText(plot_img, "Parches desechados", {margin_left + 40, stripe_discarded_top.y + 60}, cv::FONT_HERSHEY_SIMPLEX, 1.2, {0,0,200}, 2);
    }
    
    // Franja celeste RD_12dB (de 7dB a 17dB)
    cv::Rect stripe_12db(margin_left, map_coords(0, 17).y, plot_area_width, map_coords(0, 7).y - map_coords(0, 17).y);
    cv::rectangle(plot_img, stripe_12db, light_blue, -1);
    if (show_stripe_labels) {
        cv::putText(plot_img, "Polinomio 1", {margin_left + plot_area_width/4 - 60, stripe_12db.y + stripe_12db.height/2 + 20}, cv::FONT_HERSHEY_SIMPLEX, 1.5, {0,0,0}, 3); // Etiqueta Polinomio 1
        cv::putText(plot_img, "Parches usados para RD_12dB", {margin_left + 40, stripe_12db.y + 60}, cv::FONT_HERSHEY_SIMPLEX, 1.2, {0,0,0}, 2);
    }

    // Franja blanca central (de 5dB a 7dB) - "Parches desechados"
    cv::Rect stripe_discarded_middle(margin_left, map_coords(0, 7).y, plot_area_width, map_coords(0, 5).y - map_coords(0, 7).y);
    cv::rectangle(plot_img, stripe_discarded_middle, cv::Scalar(255,255,255), -1);
    if (show_stripe_labels) {
        cv::putText(plot_img, "Parches desechados", {margin_left + 40, stripe_discarded_middle.y + 60}, cv::FONT_HERSHEY_SIMPLEX, 1.2, {0,0,200}, 2);
    }

    // Franja celeste RD_0dB (de -5dB a 5dB)
    cv::Rect stripe_0db(margin_left, map_coords(0, 5).y, plot_area_width, map_coords(0, -5).y - map_coords(0, 5).y);
    cv::rectangle(plot_img, stripe_0db, light_blue, -1);
    if (show_stripe_labels) {
        cv::putText(plot_img, "Polinomio 2", {margin_left + plot_area_width/4 - 60, stripe_0db.y + stripe_0db.height/2 + 20}, cv::FONT_HERSHEY_SIMPLEX, 1.5, {0,0,0}, 3); // Etiqueta Polinomio 2
        cv::putText(plot_img, "Parches usados para RD_0dB", {margin_left + 40, stripe_0db.y + 60}, cv::FONT_HERSHEY_SIMPLEX, 1.2, {0,0,0}, 2);
    }

    // Franja blanca inferior (de min_db a -5dB) - "Parches desechados"
    cv::Rect stripe_discarded_bottom(margin_left, map_coords(0, -5).y, plot_area_width, (PLOT_HEIGHT - margin_bottom) - map_coords(0, -5).y);
    cv::rectangle(plot_img, stripe_discarded_bottom, cv::Scalar(255,255,255), -1);
    if (show_stripe_labels) {
        cv::putText(plot_img, "Parches desechados", {margin_left + 40, stripe_discarded_bottom.y + 60}, cv::FONT_HERSHEY_SIMPLEX, 1.2, {0,0,200}, 2);
    }

    // Rejilla y etiquetas de los ejes
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        cv::line(plot_img, map_coords(ev, bounds.at("min_db")), map_coords(ev, bounds.at("max_db")), {220,220,220}, 2);
        cv::putText(plot_img, std::to_string((int)ev), {map_coords(ev, bounds.at("min_db")).x-15, PLOT_HEIGHT-margin_bottom+45}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);
    }
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        cv::line(plot_img, map_coords(bounds.at("min_ev"), db), map_coords(bounds.at("max_ev"), db), {220,220,220}, 2);
        cv::putText(plot_img, std::to_string((int)db), {margin_left-70, map_coords(bounds.at("min_ev"), db).y+10}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);
    }
    
    // Líneas de referencia discontinuas
    DrawDashedLine(plot_img, map_coords(bounds.at("min_ev"), 12.0), map_coords(bounds.at("max_ev"), 12.0), cv::Scalar(0, 0, 0), 2);
    cv::putText(plot_img, "Photographic DR (SNR > 12dB)", {map_coords(bounds.at("min_ev"), 12.0).x + 40, map_coords(bounds.at("min_ev"), 12.0).y - 10}, cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
    
    DrawDashedLine(plot_img, map_coords(bounds.at("min_ev"), 0.0), map_coords(bounds.at("max_ev"), 0.0), cv::Scalar(0, 0, 0), 2);
    cv::putText(plot_img, "Engineering DR (SNR > 0dB)", {map_coords(bounds.at("min_ev"), 0.0).x + 40, map_coords(bounds.at("min_ev"), 0.0).y - 10}, cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);

    // Borde principal del área de trazado
    cv::Rect plot_rect(margin_left, margin_top, plot_area_width, plot_area_height);
    cv::rectangle(plot_img, plot_rect, {0,0,0}, 3);
    
    // Títulos de los ejes y del gráfico
    cv::putText(plot_img, "RAW Exposure (EV)", {PLOT_WIDTH/2-150, PLOT_HEIGHT-50}, cv::FONT_HERSHEY_SIMPLEX, 1.2, {0,0,0}, 3);
    cv::putText(plot_img, title, {PLOT_WIDTH/2-300, margin_top-60}, cv::FONT_HERSHEY_SIMPLEX, 2.0, {0,0,0}, 3);

    // Etiqueta del eje Y
    std::string y_label_text = "SNR (dB)";
    cv::Size text_size = cv::getTextSize(y_label_text, cv::FONT_HERSHEY_SIMPLEX, 1.2, 3, nullptr);
    cv::Point text_origin(60, margin_top + (plot_area_height + text_size.width) / 2);
    cv::Mat text_canvas = cv::Mat::zeros(text_size.height + 20, text_size.width + 20, CV_8UC3); // Lienzo temporal en blanco y negro
    cv::putText(text_canvas, y_label_text, cv::Point(10, text_size.height + 10), cv::FONT_HERSHEY_SIMPLEX, 1.2, {255,255,255}, 3);
    cv::rotate(text_canvas, text_canvas, cv::ROTATE_90_COUNTERCLOCKWISE);
    cv::Mat text_roi = plot_img(cv::Rect(text_origin.x, text_origin.y - text_canvas.rows, text_canvas.cols, text_canvas.rows));
    text_roi.setTo(cv::Scalar(0,0,0), text_canvas); // Pega el texto en negro sobre el imagen principal
}


// --- FUNCIÓN AUXILIAR 2: DIBUJA LAS CURVAS, PUNTOS Y ETIQUETAS DE INTERSECCIÓN ---
void DrawCurvesAndData(
    cv::Mat& plot_img,
    const std::vector<CurveData>& curves,
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

    for (size_t i = 0; i < curves.size(); ++i) {
        const auto& curve = curves[i];
        if (curve.signal_ev.empty()) continue;
        
        cv::Scalar curve_line_color(0, 0, 200); // Rojo oscuro BGR para la línea de la curva
        cv::Scalar curve_point_color(200, 0, 0); // Azul BGR para los puntos de la curva

        auto min_max_ev_it = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        double local_min_ev = *(min_max_ev_it.first);
        double local_max_ev = *(min_max_ev_it.second);

        // Dibujar la línea del polinomio
        std::vector<cv::Point> poly_points;
        for (double ev = local_min_ev; ev <= local_max_ev; ev += 0.05) { // Más puntos para una curva más suave
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) { snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j); }
            poly_points.push_back(map_coords(ev, snr_poly));
        }
        cv::polylines(plot_img, poly_points, false, curve_line_color, 2, cv::LINE_AA); // Grosor 2 para la línea

        // Dibujar los puntos de datos
        for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
            cv::circle(plot_img, map_coords(curve.signal_ev[j], curve.snr_db[j]), 4, curve_point_color, -1, cv::LINE_AA); // Radio 4 para los puntos
        }

        // Etiqueta de la curva (nombre del fichero, en rojo)
        std::string label = fs::path(curve.name).stem().string();
        cv::Point label_pos_end = map_coords(curve.signal_ev.back(), curve.snr_db.back());
        cv::putText(plot_img, label, {label_pos_end.x - 40, label_pos_end.y - 30}, cv::FONT_HERSHEY_SIMPLEX, 1.0, curve_line_color, 2, cv::LINE_AA);

        // Etiquetas de intersección con 12dB y 0dB
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

// --- FUNCIÓN PÚBLICA 1: GRÁFICO INDIVIDUAL ---
void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs)
{
    cv::Mat plot_img(PLOT_HEIGHT, PLOT_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));

    double min_ev_data = signal_ev.empty() ? 0 : *std::min_element(signal_ev.begin(), signal_ev.end());
    double max_ev_data = signal_ev.empty() ? 0 : *std::max_element(signal_ev.end(), signal_ev.end());
    
    // Rango DB fijo para uniformidad
    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_data) - 1.0;
    bounds["max_ev"] = ceil(max_ev_data) + 1.0;
    bounds["min_db"] = -15.0; // Fijo para consistencia con el resumen y las franjas
    bounds["max_db"] = 25.0;  // Fijo para consistencia con el resumen y las franjas

    DrawPlotBase(plot_img, "SNR Curve - " + image_title, bounds, true); // show_stripe_labels = true para individuales también

    std::vector<CurveData> single_curve_vec = {{image_title, signal_ev, snr_db, poly_coeffs}};
    DrawCurvesAndData(plot_img, single_curve_vec, bounds);
    
    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Plot saved to: " << output_filename << std::endl;
}

// --- FUNCIÓN PÚBLICA 2: GRÁFICO RESUMEN ---
void GenerateSummaryPlot(
    const std::string& output_filename,
    const std::vector<CurveData>& all_curves)
{
    cv::Mat plot_img(PLOT_HEIGHT, PLOT_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));

    double min_ev_global = 1e6, max_ev_global = -1e6;
    if (all_curves.empty()) return;
    for (const auto& curve : all_curves) {
        if (!curve.signal_ev.empty()) {
            min_ev_global = std::min(min_ev_global, *std::min_element(curve.signal_ev.begin(), curve.signal_ev.end()));
            max_ev_global = std::max(max_ev_global, *std::max_element(curve.signal_ev.begin(), curve.signal_ev.end()));
        }
    }
    
    // Rango DB fijo y rango EV ajustado a todas las curvas
    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_global) - 1.0;
    bounds["max_ev"] = ceil(max_ev_global) + 1.0;
    bounds["min_db"] = -15.0; // Fijo para consistencia con las franjas
    bounds["max_db"] = 25.0;  // Fijo para consistencia con las franjas
    
    DrawPlotBase(plot_img, "SNR Curves - Olympus OM-1", bounds, true); // Título cambiado y show_stripe_labels = true

    DrawCurvesAndData(plot_img, all_curves, bounds);

    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Summary Plot saved to: " << output_filename << std::endl;
}