// File: src/core/setup/SensorResolution.hpp
/**
 * @file SensorResolution.hpp
 * @brief Declares the functionality for detecting sensor resolution from RAW files.
 * @details This module's single responsibility is to inspect RAW file metadata
 * to determine the sensor's resolution if it was not user-provided.
 */
#pragma once
#include <string>
#include <vector>
#include <ostream>

/**
 * @brief Detects sensor resolution from RAW metadata.
 * @details Iterates through files to find a valid resolution, first from specific
 * metadata tags, falling back to image dimensions as a secondary source.
 * @param input_files The list of input file paths.
 * @param log_stream Stream for logging messages.
 * @return The detected sensor resolution in megapixels, or 0.0 if not found.
 */
double DetectSensorResolution(const std::vector<std::string>& input_files, std::ostream& log_stream);