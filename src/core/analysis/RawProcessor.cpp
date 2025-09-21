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

std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "Calculating black level from: " << filename << "..." << std::endl;
    RawFile dark_file(filename);
    if (!dark_file.Load()) return std::nullopt;

    cv::Mat raw_img = dark_file.GetRawImage();
    if (raw_img.empty()) return std::nullopt;

    // Convert cv::Mat to vector for statistical analysis
    std::vector<double> pixels;
    pixels.reserve(raw_img.total());
    raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

    double mean_value = CalculateMean(pixels);
    log_stream << "Black level obtained: " << std::fixed << std::setprecision(2) << mean_value << std::endl;
    return mean_value;
}

std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "Calculating saturation point from: " << filename << "..." << std::endl;
    RawFile sat_file(filename);
    if (!sat_file.Load()) return std::nullopt;

    cv::Mat raw_img = sat_file.GetRawImage();
    if (raw_img.empty()) return std::nullopt;

    std::vector<double> pixels;
    pixels.reserve(raw_img.total());
    raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

    // Using 5th percentile of the brightest pixels to avoid sensor defects
    double quantile_value = CalculateQuantile(pixels, 0.05);
    log_stream << "Saturation point obtained (5th percentile): " << std::fixed << std::setprecision(2) << quantile_value << std::endl;
    return quantile_value;
}

std::optional<double> OldProcessSaturationFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "Calculating saturation point from: " << filename << "..." << std::endl;
    RawFile sat_file(filename);
    if (!sat_file.Load()) return std::nullopt;

    //int bit_depth = sat_file.GetBitDepth();
    int bit_depth = 0;
    if (bit_depth == 0) {
        log_stream << "[WARNING] Could not determine bit depth from RAW metadata. Assuming 14-bit." << std::endl;
        bit_depth = 14;
    }

    cv::Mat raw_img = sat_file.GetRawImage();
    if (raw_img.empty()) return std::nullopt;

    std::vector<double> pixels;
    pixels.reserve(raw_img.total());
    raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

    // Using 5th percentile of the brightest pixels to avoid sensor defects
    double quantile_value = CalculateQuantile(pixels, 0.05);

    // Log the detected bit depth and expected saturation value
    double expected_saturation = (1 << bit_depth) - 1; // 2^bit_depth - 1
    log_stream << "Detected bit depth: " << bit_depth << " bits" << std::endl;
    log_stream << "Expected saturation level: " << expected_saturation << std::endl;
    log_stream << "Measured saturation (5th percentile): " << std::fixed << std::setprecision(2) << quantile_value << std::endl;

    // If measured value is suspiciously low (e.g., ~4095 for 14-bit), warn user
    if (bit_depth == 14 && quantile_value < 10000) {
        log_stream << "[WARNING] Measured saturation (" << quantile_value << ") is much lower than expected (" << expected_saturation << "). "
                      "This may indicate underexposure or non-linear processing." << std::endl;
    }

    return quantile_value;
}