// File: src/core/engine/Processing.hpp
/**
 * @file src/core/engine/Processing.hpp
 * @brief Defines the core file processing logic and data structures for results.
 */
#pragma once
#include "../analysis/Analysis.hpp"
#include "raw/RawFile.hpp"
#include "../utils/PathManager.hpp"
#include <vector>
#include <atomic>
#include <optional>
#include <map>

namespace DynaRange {

namespace EngineConfig {
    /**
     * @brief Compile-time switch to control keystone calculation optimization.
     */
    constexpr bool OPTIMIZE_KEYSTONE_CALCULATION = true;
}
}

/**
 * @struct AnalysisParameters
 * @brief (New struct) Encapsulates all necessary parameters for the core analysis phase.
 * @details This struct decouples the analysis engine from the ProgramOptions struct
 * used for argument parsing, improving modularity.
 */
struct AnalysisParameters {
    // Calibration values
    double dark_value;
    double saturation_value;

    // Core analysis settings
    int poly_order;
    double dr_normalization_mpx;
    std::vector<double> snr_thresholds_db;
    double patch_ratio;
    double sensor_resolution_mpx;

    // Chart geometry settings
    std::vector<double> chart_coords;
    int chart_patches_m;
    int chart_patches_n;

    // Channel selection
    RawChannelSelection raw_channels;
    
    // Output & Reporting settings
    std::string print_patch_filename;
    std::map<std::string, std::string> plot_labels;
    std::string generated_command;

    /**
     * @brief The index of the RAW file in the sorted list to be used as the
     * source for corner detection and debug patch image generation.
     */
    int source_image_index = 0;
};
/**
 * @struct SingleFileResult
 * @brief Holds the analysis results for a single RAW file.
 */
struct SingleFileResult {
    DynamicRangeResult dr_result; ///< The calculated dynamic range values.
    CurveData curve_data;
    ///< The data required to plot the SNR curve.
    cv::Mat final_debug_image;    ///< Debug image showing detected patches.
};
/**
 * @struct ProcessingResult
 * @brief Aggregates the analysis results from all processed files.
 */
struct ProcessingResult {
    std::vector<DynamicRangeResult> dr_results; ///< Collection of DR results for each file.
    std::vector<CurveData> curve_data;
    std::optional<cv::Mat> debug_patch_image;
    ///< The final debug image for --print-patches.
};

/**
 * @brief (Modified function) Processes a list of RAW files to analyze their dynamic range.
 * @param params The consolidated analysis parameters.
 * @param paths The PathManager for resolving output paths.
 * @param log_stream The output stream for logging messages.
 * @param cancel_flag Canceled. Try closing app.
 * @param raw_files A vector of pre-loaded RawFile objects to be processed.
 * @return A ProcessingResult struct containing the aggregated results.
 */
ProcessingResult ProcessFiles(
    const AnalysisParameters& params,
    const PathManager& paths,
    std::ostream& log_stream,
    const std::atomic<bool>& cancel_flag,
    const std::vector<RawFile>& raw_files);