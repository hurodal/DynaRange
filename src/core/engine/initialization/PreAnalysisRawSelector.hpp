// File: src/core/engine/initialization/PreAnalysisRawSelector.hpp
/**
 * @file PreAnalysisRawSelector.hpp
 * @brief Declares a component for selecting the optimal source RAW file for detection tasks.
 * @details This module's single responsibility is to choose the best RAW file from
 * a sorted list to be used for corner detection and debug patch generation during the pre-analysis phase.
 */
#pragma once

#include "../../io/raw/RawFile.hpp"
#include <vector>
#include <ostream>

namespace DynaRange::Engine::Initialization {

/**
 * @brief Selects the index of the best source RAW file from a sorted list for pre-analysis tasks.
 * @details The selection logic is:
 * 1. The most exposed (brightest) file that has no saturated pixels.
 * 2. If all files have saturated pixels, it falls back to the least exposed (darkest) file.
 * @param sorted_files The vector of loaded RawFile objects, pre-sorted by brightness.
 * @param saturation_value The calculated saturation level of the sensor.
 * @param log_stream The output stream for logging messages.
 * @return The zero-based index of the selected file within the vector.
 */
int SelectPreAnalysisRawIndex(
    const std::vector<RawFile>& sorted_files,
    double saturation_value,
    std::ostream& log_stream
);

} // namespace DynaRange::Engine::Initialization