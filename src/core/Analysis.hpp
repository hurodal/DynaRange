/**
 * @file core/Analysis.hpp
 * @brief Declares high-level structures and functions for the dynamic range analysis application.
 */
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include <map>
#include <opencv2/core.hpp>
#include "Arguments.hpp"

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
 * @brief Analyzes a cropped chart image to find patches and measure their signal and noise.
 * @param imgcrop The input image, corrected for geometry and cropped to the chart area.
 * @param NCOLS The number of columns in the patch grid.
 * @param NROWS The number of rows in the patch grid.
 * @param patch_ratio The relative area of the center of each patch to sample.
 * @return A PatchAnalysisResult struct containing the signal and noise vectors.
 */
PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio);

/**
 * @brief Orchestrates the mathematical analysis from patch data to final results.
 * @details Takes the raw patch data and performs all calculation steps:
 * SNR/EV conversion, polynomial fitting, and DR calculation for all thresholds.
 * @param patch_data The result from AnalyzePatches.
 * @param opts The program options (for poly_order, thresholds, etc.).
 * @param filename The original filename for populating result structures.
 * @return A pair containing the final DynamicRangeResult and CurveData.
 */
std::pair<DynamicRangeResult, CurveData> CalculateResultsFromPatches(const PatchAnalysisResult& patch_data, const ProgramOptions& opts, const std::string& filename);

/**
 * @brief Processes a dark frame to determine the camera's black level.
 * @param filename Path to the dark frame RAW file.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the calculated black level, or std::nullopt on failure.
 */
std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream);

/**
 * @brief Processes a saturated frame to determine the camera's saturation point.
 * @param filename Path to the saturated (white) frame RAW file.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the calculated saturation level, or std::nullopt on failure.
 */
std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream);

/**
 * @brief Pre-analyzes input files to sort them by brightness.
 * @details This ensures that files are processed in order of exposure, which can
 * be important for some analyses. The file list within 'opts' is modified in place.
 * @param opts Program options, passed by reference to modify the input file list.
 * @param log_stream Stream for logging messages.
 * @return true if successful, false if no files could be processed.
 */
bool PrepareAndSortFiles(ProgramOptions& opts, std::ostream& log_stream);
