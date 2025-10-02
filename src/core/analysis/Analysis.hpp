// File: src/core/analysis/Analysis.hpp
/**
 * @file src/core/analysis/Analysis.hpp
 * @brief Declares high-level structures and functions for the dynamic range analysis application.
 */
#pragma once

#include "../arguments/ArgumentsOptions.hpp"
#include <string>
#include <vector>
#include <map>
#include <opencv2/core.hpp>

// --- STRUCTURE DEFINITIONS ---

/**
 * @enum DataSource
 * @brief Defines the possible sources of pixel data for analysis from a RAW file.
 */
enum class DataSource { R, G1, G2, B, AVG };

/**
 * @struct DynamicRangeResult
 * @brief Stores the final dynamic range results for a single file.
 */
struct DynamicRangeResult {
    std::string filename; ///< The name of the processed file.
    DataSource channel;   ///< The RAW channel this result corresponds to.
    std::map<double, double> dr_values_ev; ///< Maps SNR threshold (dB) to DR value (EV).
    int samples_R = 0;
    int samples_G1 = 0;
    int samples_G2 = 0;
    int samples_B = 0;
};

/**
 * @struct PatchAnalysisResult
 * @brief Holds the raw signal and noise data extracted from the chart patches.
 */
struct PatchAnalysisResult {
    std::vector<double> signal;
    std::vector<double> noise;
    cv::Mat image_with_patches;
    double max_pixel_value = 1.0;
};

/**
 * @struct CurveData
 * @brief Contains all necessary data to plot an SNR curve for a single file.
 */
struct CurveData {
    std::string filename;
    DataSource channel;
    std::string plot_label;
    std::string camera_model;
    std::vector<double> signal_ev;
    std::vector<double> snr_db;
    cv::Mat poly_coeffs;
    std::vector<std::pair<double, double>> curve_points;
    std::string generated_command;
    float iso_speed = 0.0f;
};

/**
 * @struct SnrCurve
 * @brief Holds the data representing a calculated Signal-to-Noise Ratio curve.
 */
struct SnrCurve {
    std::vector<double> signal_ev;
    std::vector<double> snr_db;
    cv::Mat poly_coeffs;
};

// --- HIGH-LEVEL ANALYSIS FUNCTION DECLARATIONS ---
std::pair<DynamicRangeResult, CurveData> CalculateResultsFromPatches(
    PatchAnalysisResult& patch_data,
    const ProgramOptions& opts,
    const std::string& filename,
    double camera_resolution_mpx,
    DataSource channel
);