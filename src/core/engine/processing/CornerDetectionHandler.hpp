// File: src/core/engine/processing/CornerDetectionHandler.hpp
/**
 * @file CornerDetectionHandler.hpp
 * @brief Declares a handler for the automatic chart corner detection logic.
 * @details This module encapsulates the logic for detecting the four corners of a
 * test chart from a RAW image, adhering to SRP by separating this specific
 * task from the main file processing workflow.
 */
#pragma once

#include "../../io/raw/RawFile.hpp"
#include "../../arguments/ArgumentsOptions.hpp"
#include "../../utils/PathManager.hpp"
#include <opencv2/core.hpp>
#include <vector>
#include <optional>
#include <ostream>

namespace DynaRange::Engine::Processing {

/**
 * @brief Attempts to automatically detect chart corners if no manual coordinates are provided.
 * @param raw_files The list of loaded RAW files (the first one is used for detection).
 * @param opts The program options.
 * @param paths The PathManager for resolving debug output paths.
 * @param log_stream The output stream for logging.
 * @return An optional containing a vector of 4 corner points on success, or std::nullopt on failure or if not needed.
 */
std::optional<std::vector<cv::Point2d>> AttemptAutomaticCornerDetection(
    const std::vector<RawFile>& raw_files,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream
);

} // namespace DynaRange::Engine::Processing