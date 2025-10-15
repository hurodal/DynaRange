// File: src/core/setup/PreAnalysis.hpp
/**
 * @file src/core/setup/PreAnalysis.hpp
 * @brief Declares functions for pre-analyzing RAW files to extract essential metadata.
 * @details This module provides a reusable, core-level function to perform the initial
 * loading and analysis of RAW files, extracting brightness, ISO, and saturation info.
 * It is designed to be used by both the CLI and the GUI.
 */
#pragma once
#include <string>
#include <vector>
/**
 * @struct PreAnalysisResult
 * @brief Holds the extracted metadata for a single RAW file after pre-analysis.
 */
struct PreAnalysisResult {
    std::string filename;
    double mean_brightness = 0.0;
    float iso_speed = 0.0f;
    bool has_saturated_pixels = false;
    double saturation_value_used = 0.0; ///< The saturation value used for the saturated pixel check.
};
/**
 * @brief Pre-analyzes a list of RAW files to extract essential metadata.
 * @details This function loads each file, extracts its active area, and calculates
 * the mean brightness and a flag for saturated pixels. It is designed to be efficient
 * and safe for use in both CLI and GUI contexts.
 * @param input_files The list of input file paths to analyze.
 * @param saturation_value The sensor's saturation level used to check for saturated pixels.
 * @param log_stream An optional output stream for logging messages. If nullptr, no logging occurs.
 * @return A vector of PreAnalysisResult structs for successfully processed files.
 *         If a file fails to load or process, it is simply omitted from the result.
 */
std::vector<PreAnalysisResult> PreAnalyzeRawFiles(
    const std::vector<std::string>& input_files,
    double saturation_value,
    std::ostream* log_stream = nullptr);