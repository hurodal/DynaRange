/**
 * @file src/core/engine/Normalization.cpp
 * @brief Implements SNR normalization and validation logic for dynamic range analysis.
 */
#include "Normalization.hpp"

// Helper to read environment variable as integer
static bool IsDebugEnabled() {
    const char* env = std::getenv("DYNA_RANGE_DEBUG");
    return env != nullptr && std::string(env) == "1";
}

namespace DynaRange {

void NormalizeAndValidateSNR(ProcessingResult& results, const ProgramOptions& opts, std::ostream& log_stream) {
    const double THRESHOLD_DB = 12.0; // Fixed by spec for photo DR
    for (auto& curve : results.curve_data) {
        // Skip if no data
        if (curve.snr_db.empty()) {
            continue;
        }

        // Find min/max SNR in dB (already normalized in CalculateSnrCurve)
        double max_snr_db = *std::max_element(curve.snr_db.begin(), curve.snr_db.end());
        double min_snr_db = *std::min_element(curve.snr_db.begin(), curve.snr_db.end());

        if (IsDebugEnabled()) {
            log_stream << "DEBUG: ISO=" << curve.iso_speed
                       << " | min_snr_db=" << min_snr_db
                       << " | max_snr_db=" << max_snr_db
                       << " | cam_res_mpx=" << opts.sensor_resolution_mpx
                       << " | target_mpx=" << opts.dr_normalization_mpx
                       << "\n";
        }

        // Validate sufficiency using UNNORMALIZED values (they are already normalized in CalculateSnrCurve)
        bool sufficient = (min_snr_db < THRESHOLD_DB && max_snr_db > THRESHOLD_DB);
        if (!sufficient) {
            if (IsDebugEnabled()) {
                log_stream << "DEBUG:   VALIDATION FAILED: min_db=" << min_snr_db
                           << " < " << THRESHOLD_DB << " ? " << (min_snr_db < THRESHOLD_DB)
                           << " | max_db=" << max_snr_db
                           << " > " << THRESHOLD_DB << " ? " << (max_snr_db > THRESHOLD_DB)
                           << "\n";
            }
            log_stream << "Warning: insufficient data to calculate "
                       << THRESHOLD_DB << "dB dynamic range at "
                       << opts.dr_normalization_mpx << "Mpx normalization. "
                       << "Test chart was subexposed/underexposed.\n";
        }
        // ¡NO APLICAR NINGÚN OFFSET! Todo ya está normalizado en CalculateSnrCurve.
    }
}

} // namespace DynaRange