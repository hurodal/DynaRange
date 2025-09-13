// File: core/Analysis.hpp
#pragma once

#include "Arguments.hpp" 
#include "ImageProcessing.hpp"
#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include <map>
#include <opencv2/core.hpp>

/**
 * @file Analysis.hpp
 * @brief Declares high-level structures and functions for the dynamic range analysis application.
 */

// --- STRUCTURE DEFINITIONS ---

struct DynamicRangeResult {
    std::string filename;
    // Maps an SNR threshold (in dB) to its calculated Dynamic Range value (in EV).
    std::map<double, double> dr_values_ev; 
    int patches_used;
};

struct PatchAnalysisResult {
    std::vector<double> signal;
    std::vector<double> noise;
    cv::Mat image_with_patches;
};

struct CurveData {
    std::string filename;
    std::string camera_model;
    std::vector<double> signal_ev;
    std::vector<double> snr_db;
    cv::Mat poly_coeffs;
    std::string generated_command;
};

// --- HIGH-LEVEL ANALYSIS FUNCTION DECLARATIONS ---

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio);
std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream);
std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream);
bool PrepareAndSortFiles(ProgramOptions& opts, std::ostream& log_stream);
