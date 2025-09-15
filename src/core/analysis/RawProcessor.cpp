// File: src/core/analysis/RawProcessor.cpp
/**
 * @file src/core/analysis/RawProcessor.cpp
 * @brief Implements processing of dark and saturation frames for sensor calibration.
 */
#include "RawProcessor.hpp"
#include "../io/RawFile.hpp"
#include "../math/Math.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating black level from: " << filename << "..." << std::endl;
    RawFile dark_file(filename);
    if (!dark_file.Load()) return std::nullopt;

    cv::Mat raw_img = dark_file.GetRawImage();
    if (raw_img.empty()) return std::nullopt;

    // Convert cv::Mat to vector for statistical analysis
    std::vector<double> pixels;
    pixels.reserve(raw_img.total());
    raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

    double mean_value = CalculateMean(pixels);
    log_stream << "[INFO] -> Black level obtained: " << std::fixed << std::setprecision(2) << mean_value << std::endl;
    return mean_value;
}

std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating saturation point from: " << filename << "..." << std::endl;
    RawFile sat_file(filename);
    if (!sat_file.Load()) return std::nullopt;

    cv::Mat raw_img = sat_file.GetRawImage();
    if (raw_img.empty()) return std::nullopt;

    std::vector<double> pixels;
    pixels.reserve(raw_img.total());
    raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

    // Using 5th percentile of the brightest pixels to avoid sensor defects
    double quantile_value = CalculateQuantile(pixels, 0.05);
    log_stream << "[INFO] -> Saturation point obtained (5th percentile): " << std::fixed << std::setprecision(2) << quantile_value << std::endl;
    return quantile_value;
}