// File: src/core/engine/Normalization.hpp
/**
 * @file src/core/engine/Normalization.hpp
 * @brief Declares functions for SNR normalization and validation before DR calculation.
 */
#pragma once

#include "../arguments/ProgramOptions.hpp"
#include "../engine/Processing.hpp" 
#include <ostream>

namespace DynaRange {

    /**
     * @brief Validates that each ISO's SNR data is sufficient for DR calculation at the target normalization,
     *        and applies SNR normalization based on sensor resolution and target Mpx.
     *
     * This function:
     * - Computes min/max linear SNR per ISO from signal and noise vectors.
     * - Checks if the normalized SNR range spans across the 12dB threshold (required for DR calculation).
     * - If insufficient data, marks the ISO as invalid and logs a warning.
     * - Normalizes all SNR values using: 
     *   `SNR_normalized = SNR_per_pixel * sqrt(sensor_mpx / target_mpx)`
     *   and reconstructs the signal vector accordingly.
     *
     * @param results The processing results containing signal and noise vectors per ISO.
     * @param opts The program options including sensor_resolution_mpx and dr_normalization_mpx.
     * @param log_stream Output stream for warnings and messages.
     */
    void NormalizeAndValidateSNR(ProcessingResult& results, const ProgramOptions& opts, std::ostream& log_stream);

} // namespace DynaRange