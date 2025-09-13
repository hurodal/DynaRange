/**
 * @file core/Analysis.hpp
 * @brief Declares high-level structures and functions for the dynamic range analysis application.
 */
#pragma once

#include "Arguments.hpp" 
#include "ImageProcessing.hpp"
#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include <map>
#include <opencv2/core.hpp>

// --- STRUCTURE DEFINITIONS ---

/**
 * @struct DynamicRangeResult
 * @brief Stores the final dynamic range results for a single file.
 */
struct DynamicRangeResult {
    std::string filename; ///< The name of the processed file.
    /// @brief Maps an SNR threshold (in dB) to its calculated Dynamic Range value (in EV).
    std::map<double, double> dr_values_ev; 
    int patches_used; ///< The number of valid patches found and used in the analysis.
};

/**
 * @struct PatchAnalysisResult
 * @brief Holds the raw signal and noise data extracted from the chart patches.
 */
struct PatchAnalysisResult {
    std::vector<double> signal; ///< A vector of mean signal values from each patch.
    std::vector<double> noise;  ///< A vector of noise (stddev) values from each patch.
    cv::Mat image_with_patches; ///< A debug image showing the detected patches.
};

/**
 * @struct CurveData
 * @brief Contains all necessary data to plot an SNR curve for a single file.
 */
struct CurveData {
    std::string filename; ///< The name of the processed file.
    std::string camera_model; ///< The camera model name, extracted from metadata.
    std::vector<double> signal_ev; ///< The signal values converted to EV.
    std::vector<double> snr_db;    ///< The SNR values in dB.
    cv::Mat poly_coeffs;           ///< The coefficients of the fitted polynomial curve.
    std::string generated_command; ///< The command string used to generate the plot.
};

// --- HIGH-LEVEL ANALYSIS FUNCTION DECLARATIONS ---

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio);
std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream);
std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream);
bool PrepareAndSortFiles(ProgramOptions& opts, std::ostream& log_stream);