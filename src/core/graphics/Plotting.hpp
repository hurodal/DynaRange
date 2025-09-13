/**
 * @file core/graphics/Plotting.hpp
 * @brief Declares high-level functions for generating complete plot images.
 */
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>

#include "Drawing.hpp"

/**
 * @brief Generates and saves a single SNR plot for one RAW file.
 * @details This function orchestrates the plot creation: it sets up a cairo surface,
 * calculates plot boundaries, calls the low-level drawing functions, and saves
 * the result to a PNG file.
 * @param output_filename The full path for the output PNG file.
 * @param image_title The title for the specific image being plotted.
 * @param signal_ev A vector of signal values in EV.
 * @param snr_db A vector of SNR values in dB.
 * @param poly_coeffs The coefficients of the fitted polynomial curve.
 * @param opts The program options, used for plot_mode and command text.
 * @param log_stream The stream for logging messages.
 */
void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs,
    const ProgramOptions& opts,
    std::ostream& log_stream 
);

/**
 * @brief Generates and saves a summary plot containing all SNR curves.
 * @details Similar to GenerateSnrPlot, but aggregates all curves onto a single
 * image to provide a summary view.
 * @param output_dir The directory where the summary plot PNG will be saved.
 * @param camera_name The name of the camera, used in the plot title.
 * @param all_curves A vector containing the CurveData for all processed files.
 * @param opts The program options.
 * @param log_stream The stream for logging messages.
 * @return An optional string containing the path to the generated plot, or std::nullopt.
 */
std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_dir,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    const ProgramOptions& opts,
    std::ostream& log_stream 
);