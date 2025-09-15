// File: src/core/analysis/RawProcessor.hpp
/**
 * @file src/core/analysis/RawProcessor.hpp
 * @brief Declares functions to process dark and saturation frames for black/saturation levels.
 */
#pragma once

#include <string>
#include <optional>
#include <ostream>

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