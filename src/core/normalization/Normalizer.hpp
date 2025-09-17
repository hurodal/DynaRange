// File: src/core/normalization/Normalizer.hpp
/**
 * @file src/core/normalization/Normalizer.hpp
 * @brief Provides utilities for normalizing SNR values based on target resolution.
 */
#pragma once

namespace DynaRange {
    /**
     * @brief Normalizes an SNR value from input resolution to target resolution.
     * 
     * Formula: SNR_normalized = SNR_per_pixel * sqrt(input_mpx / target_mpx)
     * If target_mpx <= 0, returns SNR_per_pixel (no normalization).
     * 
     * @param snr_per_pixel The raw SNR value (linear scale, not dB).
     * @param input_mpx The sensor resolution in megapixels (e.g., 24.0).
     * @param target_mpx The desired output normalization resolution (e.g., 8.0).
     * @return The normalized SNR value.
     */
    double NormalizeSNR(double snr_per_pixel, double input_mpx, double target_mpx);

    /**
     * @brief Checks if the measured SNR range is sufficient to calculate dynamic range at target normalization.
     * 
     * For a valid DR calculation at threshold_db (typically 12dB), we require:
     * - At least one patch with normalized SNR_dB < threshold_db
     * - At least one patch with normalized SNR_dB > threshold_db
     * 
     * @param max_snr_linear Maximum linear SNR from patches.
     * @param min_snr_linear Minimum linear SNR from patches.
     * @param cam_resolution_mpx Sensor resolution in megapixels.
     * @param target_mpx Target normalization resolution in megapixels.
     * @param threshold_db The SNR threshold in dB used for DR calculation (e.g., 12.0).
     * @return true if data is sufficient, false otherwise.
     */
    bool HasSufficientDataForDR(double max_snr_linear, double min_snr_linear,
                                double cam_resolution_mpx, double target_mpx,
                                double threshold_db = 12.0);
}