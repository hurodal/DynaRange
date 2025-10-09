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
#include "../arguments/ArgumentsOptions.hpp"
#include "../utils/PathManager.hpp"
#include "../analysis/Analysis.hpp"

/**
 * @brief Generates and saves a single SNR plot for one RAW file.
 * @details This function creates a complete PNG image containing a styled plot base,
 * the fitted polynomial curve, the raw data points, a curve label, and a timestamp.
 * An optional command-line string can also be displayed.
 * @param output_filename The full path for the output PNG file.
 * @param plot_title The main title for the plot (e.g., "iso00200.dng (OM-1, ISO 200)").
 * @param curve_label The simple label to draw on the curve itself (e.g., "ISO 200").
 * @param channel R,G1,G2 or B enum.
 * @param signal_ev A vector of signal values in EV.
 * @param snr_db A vector of SNR values in dB.
 * @param poly_coeffs The coefficients of the fitted polynomial curve.
 * @param dr_result The dynamic range results, used for drawing intersection labels.
 * @param opts The program options, used for plot_mode and command text.
 * @param log_stream The stream for logging messages.
 */
void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& plot_title,
    const std::string& curve_label,
    DataSource channel,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs,
    const DynamicRangeResult& dr_result,
    const ProgramOptions& opts,
    std::ostream& log_stream 
);

/**
 * @brief Generates and saves a summary plot containing all SNR curves.
 * @details Creates a comprehensive overview plot comparing all processed RAW files.
 * It uses the same styling as individual plots but overlays multiple curves on
 * the same axes and includes a timestamp.
 * @param output_filename The full path where the summary plot PNG will be saved.
 * @param camera_name The name of the camera, used in the plot title.
 * @param all_curves A vector containing the CurveData for all processed files.
 * @param all_results A vector containing the DynamicRangeResult for all processed files.
 * @param opts The program options.
 * @param log_stream The stream for logging messages.
 * @return An optional string containing the path to the generated plot, or std::nullopt.
 */
std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_filename,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    std::ostream& log_stream 
);

/**
 * @brief Generates and saves all individual plots.
 * @param all_curves_data Vector of all curve data.
 * @param all_dr_results Vector of all DR results.
 * @param opts The program options.
 * @param paths The PathManager for generating output paths.
 * @param log_stream Stream for logging.
 * @return A map where the key is the source RAW filename and the value is the path to the generated plot.
 */
std::map<std::string, std::string> GenerateIndividualPlots(
    const std::vector<CurveData>& all_curves_data,
    const std::vector<DynamicRangeResult>& all_dr_results,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream);