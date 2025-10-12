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

enum class DataSource { R, G1, G2, B, AVG };
/**
 * @struct PointData
 * @brief Holds the data for a single plotted point, including its source channel.
 */
struct PointData {
    double ev;
    double snr_db;
    DataSource channel;
};

struct DynamicRangeResult {
    std::string filename;
    DataSource channel;
    float iso_speed = 0.0f;
    std::map<double, double> dr_values_ev;
    int samples_R = 0;
    int samples_G1 = 0;
    int samples_G2 = 0;
    int samples_B = 0;
};

struct PatchAnalysisResult {
    std::vector<double> signal;
    std::vector<double> noise;
    // For AVG analysis, this will store the origin channel of each point.
    std::vector<DataSource> channels;
    cv::Mat image_with_patches;
    double max_pixel_value = 1.0;
};

struct CurveData {
    std::string filename;
    DataSource channel;
    std::string plot_label;
    std::string camera_model;
    std::vector<PointData> points; // Replaced separate signal_ev and snr_db vectors
    cv::Mat poly_coeffs;
    std::vector<std::pair<double, double>> curve_points;
    std::string generated_command;
    float iso_speed = 0.0f;
};

struct SnrCurve {
    std::vector<PointData> points;
    // Replaced separate signal_ev and snr_db vectors
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