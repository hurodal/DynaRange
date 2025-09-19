// File: src/core/engine/Validation.hpp
/**
 * @file src/core/engine/Validation.hpp
 * @brief Declares functions for validating SNR results before final reporting.
 */
#pragma once

#include "../arguments/ProgramOptions.hpp"
#include "../engine/Processing.hpp" 
#include <ostream>

namespace DynaRange {
    /**
     * @brief Validates that each ISO's SNR data is sufficient for a reliable DR calculation.
     *
     * This function checks if the final, normalized SNR range for a curve spans
     * across the critical 12dB threshold required for photographic DR calculation.
     * If the data is insufficient (e.g., the test chart was over/underexposed),
     * it logs a warning.
     *
     * @param results The processing results containing the final curve data.
     * @param opts The program options, used for logging context.
     * @param log_stream Output stream for warnings and messages.
     */
    void ValidateSnrResults(const ProcessingResult& results, const ProgramOptions& opts, std::ostream& log_stream);

} // namespace DynaRange