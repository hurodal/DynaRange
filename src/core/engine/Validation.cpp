// File: src/core/engine/Validation.cpp
/**
 * @file src/core/engine/Validation.cpp
 * @brief Implements SNR validation logic.
 */
#include "Validation.hpp"
#include <algorithm> // For std::minmax_element
#include <string>    // For std::string, std::getenv
#include <iostream>
#include <libintl.h>

#define _(string) gettext(string)

// Helper to read environment variable as integer
static bool IsDebugEnabled() {
    const char* env = std::getenv("DYNA_RANGE_DEBUG");
    return env != nullptr && std::string(env) == "1";
}

namespace DynaRange {

void ValidateSnrResults(const ProcessingResult& results, const ProgramOptions& opts, std::ostream& log_stream) {
    const double THRESHOLD_DB = 12.0; // Fixed by spec for photographic DR

    for (const auto& curve : results.curve_data) {
        // Skip if there's no data for this curve
        if (curve.snr_db.empty()) {
            continue;
        }

        // Find min/max SNR in dB.
        // These values are already normalized from the CurveCalculator stage.
        double max_snr_db = *std::max_element(curve.snr_db.begin(), curve.snr_db.end());
        double min_snr_db = *std::min_element(curve.snr_db.begin(), curve.snr_db.end());
        
        if (IsDebugEnabled()) {
            log_stream << "DEBUG: ISO=" << curve.iso_speed
                       << " | min_snr_db=" << min_snr_db
                       << " | max_snr_db=" << max_snr_db
                       << " | cam_res_mpx=" << opts.sensor_resolution_mpx
                       << " | target_mpx=" << opts.dr_normalization_mpx
                       << std::endl;
        }

        // A valid DR calculation requires data points on both sides of the threshold.
        bool sufficient_data = (min_snr_db < THRESHOLD_DB && max_snr_db > THRESHOLD_DB);
        
        if (!sufficient_data) {
            if (IsDebugEnabled()) {
                log_stream << "DEBUG:   VALIDATION FAILED: min_db=" << min_snr_db
                           << " < " << THRESHOLD_DB << " ? " << (min_snr_db < THRESHOLD_DB)
                           << " | max_db=" << max_snr_db
                           << " > " << THRESHOLD_DB << " ? " << (max_snr_db > THRESHOLD_DB)
                           << std::endl;
            }
            log_stream << _("Warning: insufficient data to calculate ")
                       << THRESHOLD_DB << _("dB dynamic range at ")
                       << opts.dr_normalization_mpx << _("Mpx normalization. ")
                       << _("Test chart may have been over/underexposed for this ISO.") << std::endl;
        }
        
        // Note: No normalization offset is applied here.
        // All SNR normalization
        // is handled in `CurveCalculator` before the polynomial fitting occurs.
    }
}

} // namespace DynaRange