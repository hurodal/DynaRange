// File: src/core/engine/initialization/InputFileFilter.hpp
/**
 * @file InputFileFilter.hpp
 * @brief Declares a component for filtering and cleaning the input file list.
 * @details This module adheres to SRP by encapsulating the logic for excluding
 * calibration files and removing duplicates from the initial list of input files.
 */
#pragma once

#include "../../arguments/ArgumentsOptions.hpp"
#include <ostream>

namespace DynaRange::Engine::Initialization {

/**
 * @class InputFileFilter
 * @brief Cleans the initial list of RAW input files.
 */
class InputFileFilter {
public:
    /**
     * @brief Filters the input file list in ProgramOptions.
     * @details This function performs two main actions:
     * 1. It removes any files that are specified as calibration files (dark/saturation).
     * 2. It removes any duplicate entries from the list.
     * The function modifies the ProgramOptions object directly.
     * @param opts A reference to the program options, which will be updated.
     * @param log_stream The output stream for logging messages.
     */
    void Filter(ProgramOptions& opts, std::ostream& log_stream) const;
};

} // namespace DynaRange::Engine::Initialization