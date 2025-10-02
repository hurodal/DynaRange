// File: src/core/setup/CalibrationEstimator.hpp
/**
 * @file CalibrationEstimator.hpp
 * @brief Declares functions for estimating calibration values from RAW files.
 * @details This module's responsibility is to provide default black and
 * saturation levels when they are not provided by the user.
 */
#pragma once

#include "../arguments/ArgumentsOptions.hpp"
#include "MetadataExtractor.hpp" // For FileInfo
#include <optional>
#include <ostream>
#include <vector>

namespace CalibrationEstimator {

/**
 * @brief Estimates the black level using the darkest file from the input series.
 * @param opts Program options.
 * @param file_info A vector of pre-analyzed file metadata.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the estimated black level, or std::nullopt on failure.
 */
std::optional<double> EstimateBlackLevel(const ProgramOptions& opts, const std::vector<FileInfo>& file_info, std::ostream& log_stream);

/**
 * @brief Estimates the saturation level from the highest ISO input file.
 * @details Finds the file with the highest ISO speed in the pre-analyzed list,
 * determines its bit depth, and calculates the saturation level as (2^bit_depth - 1).
 * @param opts Program options.
 * @param file_info A vector of pre-analyzed file metadata.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the estimated saturation level, or std::nullopt on failure.
 */
std::optional<double> EstimateSaturationLevel(const ProgramOptions& opts, const std::vector<FileInfo>& file_info, std::ostream& log_stream);

} // namespace CalibrationEstimator