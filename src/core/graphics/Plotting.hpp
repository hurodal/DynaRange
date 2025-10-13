// File: src/core/graphics/Plotting.hpp
/**
 * @file src/core/graphics/Plotting.hpp
 * @brief Declares high-level functions for generating complete plot images.
 * @details This module provides functions to create, style, and save SNR curve plots
 * as PNG images. It orchestrates lower-level drawing functions from PlotBase and
 * PlotData to produce the final output.
 */
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include "../utils/PathManager.hpp"
#include "../analysis/Analysis.hpp"
#include "../engine/Reporting.hpp" 

/**
 * @brief Generates and saves a summary plot containing all SNR curves.
 * @param output_filename The full path where the summary plot will be saved.
 * @param camera_name The name of the camera, used in the plot title.
 * @param all_curves A vector containing the CurveData for all processed files.
 * @param all_results A vector containing the DynamicRangeResult for all processed files.
 * @param reporting_params The consolidated parameters required for generating the plot.
 * @param log_stream The stream for logging messages.
 * @return An optional string containing the path to the generated plot, or std::nullopt.
 */
std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_filename,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    const std::vector<DynamicRangeResult>& all_results,
    const ReportingParameters& reporting_params,
    std::ostream& log_stream 
);

/**
 * @brief Generates and saves all individual plots.
 * @param all_curves_data Vector of all curve data.
 * @param all_dr_results Vector of all DR results.
 * @param reporting_params The consolidated parameters required for generating plots.
 * @param paths The PathManager for generating output paths.
 * @param log_stream Stream for logging.
 * @return A map where the key is the source RAW filename and the value is the path to the generated plot.
 */
std::map<std::string, std::string> GenerateIndividualPlots(
    const std::vector<CurveData>& all_curves_data,
    const std::vector<DynamicRangeResult>& all_dr_results,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream);