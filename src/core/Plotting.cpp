// Fichero: core/Plotting.cpp
#include "Plotting.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs)
{
    const int width = 1920, height = 1080;
    const int margin_left = 120, margin_bottom = 100, margin_top = 80, margin_right = 60;
    const int plot_area_width = width - margin_left - margin_right;
    const int plot_area_height = height - margin_top - margin_bottom;
    cv::Mat plot_img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));
    double min_ev_data = signal_ev.empty() ? 0 : *std::min_element(signal_ev.begin(), signal_ev.end());
    double max_ev_data = signal_ev.empty() ? 0 : *std::max_element(signal_ev.begin(), signal_ev.end());
    double min_db_data = snr_db.empty() ? -5 : *std::min_element(snr_db.begin(), snr_db.end());
    double max_db_data = snr_db.empty() ? 25 : *std::max_element(snr_db.begin(), snr_db.end());
    double ev_range = max_ev_data - min_ev_data;
    double db_range = max_db_data - min_db_data;
    double ev_pad = std::max(1.0, ev_range * 0.1);
    double db_pad = std::max(5.0, db_range * 0.1);
    double min_ev = floor(min_ev_data - ev_pad);
    double max_ev = ceil(max_ev_data + ev_pad);
    double min_db = floor(min_db_data - db_pad);
    double max_db = ceil(max_db_data + db_pad);
    min_db = std::min({min_db, 0.0, 12.0});
    max_db = std::max({max_db, 0.0, 12.0});
    min_db = floor(min_db / 5.0) * 5.0;
    max_db = ceil(max_db / 5.0) * 5.0;
    min_ev = floor(min_ev);
    max_ev = ceil(max_ev);

    auto map_coords = [&](double ev, double db) {
        int px = static_cast<int>(margin_left + (ev - min_ev) / (max_ev - min_ev) * plot_area_width);
        int py = static_cast<int>((height - margin_bottom) - (db - min_db) / (max_db - min_db) * plot_area_height);
        return cv::Point(px, py);
    };

    cv::Rect plot_rect(margin_left, margin_top, plot_area_width, plot_area_height);
    cv::rectangle(plot_img, plot_rect, cv::Scalar(0,0,0), 2);
    for (double ev = min_ev; ev <= max_ev; ev += 1.0) { cv::line(plot_img, map_coords(ev, min_db), map_coords(ev, max_db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)ev), {map_coords(ev, min_db).x-10, height-margin_bottom+25}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    for (double db = min_db; db <= max_db; db += 5.0) { cv::line(plot_img, map_coords(min_ev, db), map_coords(max_ev, db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)db), {margin_left-40, map_coords(min_ev, db).y+7}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    cv::putText(plot_img, "RAW Exposure (EV)", {width/2-70, height-25}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {0,0,0}, 2);
    cv::Mat y_label_img(plot_area_height, 40, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::putText(y_label_img, "SNR (dB)", cv::Point(5, plot_area_height / 2 + 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,0,0), 2);
    cv::rotate(y_label_img, y_label_img, cv::ROTATE_90_COUNTERCLOCKWISE);
    y_label_img.copyTo(plot_img(cv::Rect(20, margin_top + plot_area_height / 2 - (y_label_img.rows / 2), y_label_img.cols, y_label_img.rows)));
    cv::putText(plot_img, "SNR Curve - " + image_title, {width/2-150, margin_top-30}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);
    cv::line(plot_img, map_coords(min_ev, 12.0), map_coords(max_ev, 12.0), cv::Scalar(0, 100, 0), 2, cv::LINE_AA);
    cv::putText(plot_img, "12 dB (Photographic DR)", {map_coords(max_ev, 12.0).x - 250, map_coords(max_ev, 12.0).y - 10}, cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 100, 0), 1);
    cv::line(plot_img, map_coords(min_ev, 0.0), map_coords(max_ev, 0.0), cv::Scalar(150, 0, 0), 2, cv::LINE_AA);
    cv::putText(plot_img, "0 dB (Engineering DR)", {map_coords(max_ev, 0.0).x - 220, map_coords(max_ev, 0.0).y - 10}, cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(150, 0, 0), 1);
    std::vector<cv::Point> poly_points;
    for (int px = margin_left; px < width - margin_right; ++px) {
        double ev = min_ev + (double)(px - margin_left) / plot_area_width * (max_ev - min_ev);
        double snr_poly = 0.0;
        for (int i = 0; i < poly_coeffs.rows; ++i) { snr_poly += poly_coeffs.at<double>(i) * std::pow(ev, poly_coeffs.rows - 1 - i); }
        poly_points.push_back(map_coords(ev, snr_poly));
    }
    cv::polylines(plot_img, poly_points, false, cv::Scalar(0, 0, 200), 3, cv::LINE_AA);
    for (size_t i = 0; i < signal_ev.size(); ++i) {
        cv::circle(plot_img, map_coords(signal_ev[i], snr_db[i]), 5, cv::Scalar(255, 0, 0), -1, cv::LINE_AA);
        cv::circle(plot_img, map_coords(signal_ev[i], snr_db[i]), 5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }
    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Plot saved to: " << output_filename << std::endl;
}

void GenerateSummaryPlot(
    const std::string& output_filename,
    const std::vector<CurveData>& all_curves)
{
    const int width = 1920, height = 1080;
    const int margin_left = 120, margin_bottom = 100, margin_top = 120, margin_right = 300;
    const int plot_area_width = width - margin_left - margin_right;
    const int plot_area_height = height - margin_top - margin_bottom;
    cv::Mat plot_img = cv::Mat::ones(height, width, CV_8UC3) * 255;
    double min_ev = 1e6, max_ev = -1e6, min_db = 1e6, max_db = -1e6;
    if (all_curves.empty()) return;
    for (const auto& curve : all_curves) {
        if (!curve.signal_ev.empty()) {
            min_ev = std::min(min_ev, *std::min_element(curve.signal_ev.begin(), curve.signal_ev.end()));
            max_ev = std::max(max_ev, *std::max_element(curve.signal_ev.begin(), curve.signal_ev.end()));
        }
        if (!curve.snr_db.empty()) {
            min_db = std::min(min_db, *std::min_element(curve.snr_db.begin(), curve.snr_db.end()));
            max_db = std::max(max_db, *std::max_element(curve.snr_db.begin(), curve.snr_db.end()));
        }
    }
    min_db = floor(min_db / 5.0) * 5.0 - 5.0;
    max_db = ceil(max_db / 5.0) * 5.0 + 5.0;
    min_ev = floor(min_ev) - 1.0;
    max_ev = ceil(max_ev) + 1.0;
    auto map_coords = [&](double ev, double db) {
        int px = static_cast<int>(margin_left + (ev - min_ev) / (max_ev - min_ev) * plot_area_width);
        int py = static_cast<int>((height - margin_bottom) - (db - min_db) / (max_db - min_db) * plot_area_height);
        return cv::Point(px, py);
    };
    cv::Rect plot_rect(margin_left, margin_top, plot_area_width, plot_area_height);
    cv::rectangle(plot_img, plot_rect, cv::Scalar(255, 248, 225), -1);
    cv::rectangle(plot_img, plot_rect, {0,0,0}, 2);
    for (double ev = ceil(min_ev); ev <= floor(max_ev); ev += 1.0) { cv::line(plot_img, map_coords(ev, min_db), map_coords(ev, max_db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)ev), {map_coords(ev, min_db).x-10, height-margin_bottom+25}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    for (double db = ceil(min_db); db <= floor(max_db); db += 5.0) { cv::line(plot_img, map_coords(min_ev, db), map_coords(max_ev, db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)db), {margin_left-40, map_coords(min_ev, db).y+7}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    cv::putText(plot_img, "RAW Exposure (EV)", {width/2-70, height-25}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {0,0,0}, 2);
    cv::Mat y_label_img(plot_area_height, 40, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::putText(y_label_img, "SNR (dB)", cv::Point(5, plot_area_height / 2 + 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,0,0), 2);
    cv::rotate(y_label_img, y_label_img, cv::ROTATE_90_COUNTERCLOCKWISE);
    y_label_img.copyTo(plot_img(cv::Rect(40, margin_top + plot_area_height / 2 - (y_label_img.rows / 2), y_label_img.cols, y_label_img.rows)));
    cv::putText(plot_img, "SNR Curves Summary", {width/2-150, margin_top-30}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);
    std::vector<cv::Scalar> colors = { {200,0,0}, {0,0,200}, {0,150,0}, {0,150,150}, {150,150,0}, {150,0,150}, {0,75,150}, {100,100,100}, {200,100,0}, {0,100,200}, {100,200,0}, {100,0,200} };
    int legend_y_pos = margin_top;
    for (size_t i = 0; i < all_curves.size(); ++i) {
        const auto& curve = all_curves[i];
        if (curve.signal_ev.empty()) continue;
        cv::Scalar color = colors[i % colors.size()];
        auto min_max_ev_it = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
        double local_min_ev = *min_max_ev_it->first;
        double local_max_ev = *min_max_ev_it->second;
        std::vector<cv::Point> poly_points;
        for (double ev = local_min_ev; ev <= local_max_ev; ev += 0.05) {
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) { snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j); }
            poly_points.push_back(map_coords(ev, snr_poly));
        }
        cv::polylines(plot_img, poly_points, false, color, 2, cv::LINE_AA);
        for(size_t j = 0; j < curve.signal_ev.size(); ++j) {
            cv::circle(plot_img, map_coords(curve.signal_ev[j], curve.snr_db[j]), 3, color, -1, cv::LINE_AA);
        }
        cv::rectangle(plot_img, {width - margin_right + 10, legend_y_pos - 12}, {width - margin_right + 30, legend_y_pos + 8}, color, -1);
        cv::putText(plot_img, fs::path(curve.name).stem().string(), {width - margin_right + 40, legend_y_pos + 5}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1);
        legend_y_pos += 25;
    }
    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Summary Plot saved to: " << output_filename << std::endl;
}