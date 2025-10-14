// File: src/core/engine/processing/CornerDetectionHandler.hpp
/**
 * @file CornerDetectionHandler.hpp
 * @brief Declares a handler for the automatic chart corner detection logic.
 */
#pragma once
#include "../../io/raw/RawFile.hpp"
#include "../../utils/PathManager.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <optional>
#include <ostream>

namespace DynaRange::Engine::Processing {

/**
 * @brief Attempts to automatically detect chart corners if no manual coordinates are provided.
 * @param source_raw_file The single loaded RAW file to use for detection.
 * @param chart_coords The vector of manually provided chart coordinates.
 * @param dark_value The black level for normalization.
 * @param saturation_value The saturation level for normalization.
 * @param paths The PathManager for resolving debug output paths.
 * @param log_stream The output stream for logging.
 * @return An optional containing a vector of 4 corner points on success, or std::nullopt on failure or if not needed.
 */
std::optional<std::vector<cv::Point2d>> AttemptAutomaticCornerDetection(
    const RawFile& source_raw_file,
    const std::vector<double>& chart_coords,
    double dark_value,
    double saturation_value,
    const PathManager& paths,
    std::ostream& log_stream
);

} // namespace DynaRange::Engine::Processing