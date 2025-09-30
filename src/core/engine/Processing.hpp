// File: src/core/engine/Processing.hpp
/**
 * @file src/core/engine/Processing.hpp
 * @brief Defines the core file processing logic and data structures for results.
 */
#pragma once
#include "../analysis/Analysis.hpp" 
#include "../arguments/ArgumentsOptions.hpp"
#include <vector>
#include <Eigen/Dense>
#include <atomic>
#include <optional>

namespace DynaRange {

namespace EngineConfig {
    /**
     * @brief Compile-time switch to control keystone calculation optimization.
     */
    constexpr bool OPTIMIZE_KEYSTONE_CALCULATION = true;
}
}

/**
 * @struct SingleFileResult
 * @brief Holds the analysis results for a single RAW file.
 */
struct SingleFileResult {
    DynamicRangeResult dr_result; ///< The calculated dynamic range values.
    CurveData curve_data;         ///< The data required to plot the SNR curve.
    cv::Mat final_debug_image;    ///< Debug image showing detected patches.
};

/**
 * @struct ProcessingResult
 * @brief Aggregates the analysis results from all processed files.
 */
struct ProcessingResult {
    std::vector<DynamicRangeResult> dr_results; ///< Collection of DR results for each file.
    std::vector<CurveData> curve_data;          ///< Collection of curve data for each file.
    std::optional<cv::Mat> debug_patch_image;   ///< The final debug image for --print-patches.
};

/**
 * @brief Processes a list of RAW files to analyze their dynamic range.
 * @param opts The program options containing all configuration settings.
 * @param log_stream The output stream for logging messages.
 * @param cancel_flag Canceled. Try closing app.
 * @return A ProcessingResult struct containing the aggregated results.
 */
ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag);