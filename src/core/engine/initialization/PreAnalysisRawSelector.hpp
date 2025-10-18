// File: src/core/engine/initialization/PreAnalysisRawSelector.hpp
/**
 * @file PreAnalysisRawSelector.hpp
 * @brief Declares a component for selecting the optimal source RAW file for detection tasks.
 * @details This module's single responsibility is to choose the best RAW file from
 * a sorted list to be used for corner detection and debug patch generation during the pre-analysis phase.
 */
#pragma once

#include "../../setup/PreAnalysis.hpp" // Now needs PreAnalysisResult
#include <vector>
#include <ostream>

namespace DynaRange::Engine::Initialization {

/**
 * @brief Selects the index of the best source RAW file from a sorted list for pre-analysis tasks.
 * @details The selection logic is:
 * 1. The most exposed (brightest) file where `has_saturated_pixels` is false (less than MAX_PRE_ANALYSIS_SATURATION_RATIO saturated).
 * 2. If all files have `has_saturated_pixels` as true, it falls back to the least exposed (darkest) file.
 * @param sorted_pre_analysis_results The vector of PreAnalysisResult objects, pre-sorted by brightness (darkest first).
 * @param log_stream The output stream for logging messages.
 * @return The zero-based index of the selected file within the vector.
 */
int SelectPreAnalysisRawIndex( const std::vector<PreAnalysisResult>& sorted_pre_analysis_results, std::ostream& log_stream );

} // namespace DynaRange::Engine::Initialization