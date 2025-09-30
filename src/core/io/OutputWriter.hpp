// File: src/core/io/OutputWriter.hpp
/**
 * @file src/core/io/OutputWriter.hpp
 * @brief Declares a centralized module for writing all output files.
 * @details This module adheres to SRP by encapsulating the low-level logic of
 * writing different data types (PNG from Cairo, CSV from results) to disk.
 */
#pragma once

#include <vector>
#include <ostream>
#include <filesystem>
#include <cairo/cairo.h>
#include <opencv2/core.hpp>
#include "../analysis/Analysis.hpp"
#include "../arguments/ArgumentsOptions.hpp"

namespace fs = std::filesystem;

namespace OutputWriter {

/**
 * @brief Writes a Cairo surface to a PNG file.
 * @param surface The cairo_surface_t containing the image data.
 * @param path The full filesystem path for the output PNG.
 * @param log_stream A stream for logging success or error messages.
 * @return true on success, false on failure.
 */
bool WritePng(cairo_surface_t* surface, const fs::path& path, std::ostream& log_stream);

/**
 * @brief (New Function) Writes a floating-point OpenCV debug image to a file.
 * @details Handles the conversion from a 32-bit float [0.0, 1.0] matrix to a
 * standard 8-bit PNG image before saving.
 * @param image The cv::Mat (CV_32F) to save.
 * @param path The full filesystem path for the output PNG.
 * @param log_stream A stream for logging success or error messages.
 * @return true on success, false on failure.
 */
bool WriteDebugImage(const cv::Mat& image, const fs::path& path, std::ostream& log_stream);

/**
 * @brief Writes the analysis results to a CSV file.
 * @param all_results The vector of DynamicRangeResult structs to write.
 * @param opts The program options, used to get the SNR thresholds for the header.
 * @param path The full filesystem path for the output CSV.
 * @param log_stream A stream for logging success or error messages.
 * @return true on success, false on failure.
 */
bool WriteCsv(
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    const fs::path& path,
    std::ostream& log_stream);

} // namespace OutputWriter