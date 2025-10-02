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
 * @details The 'signal' and 'noise' vectors are non-const to allow for normalization
 * based on sensor resolution and target Mpx before curve fitting.
 * This ensures consistency between the EV axis (based on normalized signal)
 * and the SNR values (also normalized).
 */
struct PatchAnalysisResult {
    std::vector<double> signal; ///< A vector of mean signal values from each patch.
    std::vector<double> noise;  ///< A vector of noise (stddev) values from each patch.
    cv::Mat image_with_patches; ///< A debug image showing the detected patches.
    double max_pixel_value = 1.0; ///< Max pixel value before drawing overlays.
};

/**
 * @struct CurveData
 * @brief Contains all necessary data to plot an SNR curve for a single file.
 */
struct CurveData {
    std::string filename;       ///< The name of the processed file.
    std::string plot_label;     ///< The label for this curve on the plot (e.g., "ISO 200" or filename).
    std::string camera_model;   ///< The camera model name, extracted from metadata.
    std::vector<double> signal_ev; ///< The signal values converted to EV.
    std::vector<double> snr_db; ///< The SNR values in dB.
    cv::Mat poly_coeffs;        ///< The coefficients of the fitted polynomial curve.
    std::vector<std::pair<double, double>> curve_points;  // The generated points for plotting the fitted curve.
    std::string generated_command;  ///< The command string used to generate the plot.
    float iso_speed = 0.0f;     ///< The ISO speed from EXIF, used for individual plot titles.
};

/**
 * @struct SnrCurve
 * @brief Holds the data representing a calculated Signal-to-Noise Ratio curve.
 */
struct SnrCurve {
    std::vector<double> signal_ev; ///< Signal values in EV.
    std::vector<double> snr_db;    ///< Signal-to-Noise ratio values in dB.
    cv::Mat poly_coeffs;           ///< Coefficients of the polynomial fit.
};

// --- HIGH-LEVEL ANALYSIS FUNCTION DECLARATIONS ---

/**
 * @brief Orchestrates the mathematical analysis from patch data to final results.
 * @details Takes the raw patch data and performs all calculation steps:
 * SNR/EV conversion, normalization, polynomial fitting, and DR calculation.
 * @param patch_data The result from AnalyzePatches.
 * @param opts The program options (for poly_order, thresholds, normalization, etc.).
 * @param filename The original filename for populating result structures.
 * @param camera_resolution_mpx The actual resolution of the camera's sensor in megapixels.
 * @return A pair containing the final DynamicRangeResult and CurveData.
 */
std::pair<DynamicRangeResult, CurveData> CalculateResultsFromPatches(
    PatchAnalysisResult& patch_data,
    const ProgramOptions& opts,
    const std::string& filename,
    double camera_resolution_mpx
);