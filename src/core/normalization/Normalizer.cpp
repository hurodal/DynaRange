// File: src/core/normalization/Normalizer.cpp
/**
 * @file src/core/normalization/Normalizer.cpp
 * @brief Implements SNR normalization and sufficiency validation logic.
 */
#include "Normalizer.hpp"
#include <cmath>

namespace DynaRange {

    double NormalizeSNR(double snr_per_pixel, double input_mpx, double target_mpx) {
        if (target_mpx <= 0 || input_mpx <= 0) {
            return snr_per_pixel; // No normalization
        }
        const double factor = std::sqrt(input_mpx / target_mpx);
        return snr_per_pixel * factor;
    }

    bool HasSufficientDataForDR(double max_snr_linear, double min_snr_linear,
                                double cam_resolution_mpx, double target_mpx,
                                double threshold_db) {
        if (target_mpx <= 0) {
            return true; // Per-pixel mode: always sufficient
        }
        // Convert to dB: 20*log10(SNR)
        const double min_snr_db = 20.0 * std::log10(min_snr_linear);
        const double max_snr_db = 20.0 * std::log10(max_snr_linear);
        // Apply normalization: add offset in dB
        const double offset_db = 10.0 * std::log10(cam_resolution_mpx / target_mpx); // = 20 * log10(sqrt(...))
        const double min_snr_normalized_db = min_snr_db + offset_db;
        const double max_snr_normalized_db = max_snr_db + offset_db;
        // We need points both below AND above the threshold
        return (min_snr_normalized_db < threshold_db && max_snr_normalized_db > threshold_db);
    }

} // namespace DynaRange