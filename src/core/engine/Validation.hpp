// File: src/core/engine/Validation.hpp
/**
 * @file src/core/engine/Validation.hpp
 * @brief Declares functions for validating SNR results before final reporting.
 */
#pragma once

#include "processing/Processing.hpp" 
#include <ostream>

namespace DynaRange {
    /**
     * @brief Validates that each ISO's SNR data is sufficient for a reliable DR calculation.
     * @param results The processing results containing the final curve data.
     * @param params The analysis parameters, used for logging context.
     * @param log_stream Output stream for warnings and messages.
     */
    void ValidateSnrResults(const ProcessingResult& results, const AnalysisParameters& params, std::ostream& log_stream);
} // namespace DynaRange