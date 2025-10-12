// File: src/core/engine/initialization/CalibrationHandler.hpp
/**
 * @file CalibrationHandler.hpp
 * @brief Declares a component for handling sensor calibration.
 * @details This module adheres to SRP by encapsulating all logic related to
 * determining the black and saturation levels, either by estimating them or by
 * processing dedicated calibration files.
 */
#pragma once

#include "../../arguments/ArgumentsOptions.hpp"
#include "../../setup/MetadataExtractor.hpp" // For FileInfo
#include <ostream>

namespace DynaRange::Engine::Initialization {

/**
 * @class CalibrationHandler
 * @brief Manages the determination of black and saturation levels.
 */
class CalibrationHandler {
public:
    /**
     * @brief Determines the final black and saturation values.
     * @details This function follows a clear priority:
     * 1. If explicit calibration files are provided in `opts`, it processes them.
     * 2. Otherwise, if default values are being used, it attempts to estimate them.
     * 3. If estimation fails, it falls back to the hardcoded default values.
     * The function modifies the ProgramOptions object directly.
     * @param opts A reference to the program options, which will be updated.
     * @param file_info A vector of pre-analyzed file metadata for estimation.
     * @param log_stream The output stream for logging messages.
     * @return true on success, false if a fatal error occurs during processing.
     */
    bool HandleCalibration(ProgramOptions& opts, const std::vector<FileInfo>& file_info, std::ostream& log_stream) const;
};

} // namespace DynaRange::Engine::Initialization