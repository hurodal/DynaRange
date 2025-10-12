// File: src/core/engine/initialization/CalibrationHandler.cpp
/**
 * @file CalibrationHandler.cpp
 * @brief Implements the sensor calibration logic.
 */
#include "CalibrationHandler.hpp"
#include "../../analysis/RawProcessor.hpp"
#include "../../setup/CalibrationEstimator.hpp"
#include <libintl.h>

#define _(string) gettext(string)

namespace DynaRange::Engine::Initialization {

bool CalibrationHandler::HandleCalibration(ProgramOptions& opts, const std::vector<FileInfo>& file_info, std::ostream& log_stream) const
{
    // --- 1. DEFAULT CALIBRATION ESTIMATION ---
    if (opts.dark_file_path.empty() && opts.black_level_is_default) {
        log_stream << _("[INFO] Black level not specified. Attempting to estimate from RAW file...") << std::endl;
        auto estimated_black = CalibrationEstimator::EstimateBlackLevel(opts, file_info, log_stream);
        if (estimated_black) {
            opts.dark_value = *estimated_black;
        } else {
            log_stream << _("[Warning] Could not estimate black level. Using fallback default value: ") 
                       << opts.dark_value << std::endl;
        }
    }

    if (opts.sat_file_path.empty() && opts.saturation_level_is_default) {
        log_stream << _("[INFO] Saturation level not specified. Attempting to estimate from RAW file...") << std::endl;
        auto estimated_sat = CalibrationEstimator::EstimateSaturationLevel(opts, file_info, log_stream);
        if (estimated_sat) {
            opts.saturation_value = *estimated_sat;
        } else {
            log_stream << _("[Warning] Could not estimate saturation level. Using fallback default value: ")
                       << opts.saturation_value << std::endl;
        }
    }

    // --- 2. CALIBRATION FROM EXPLICIT FILES (overwrites estimates) ---
    if (!opts.dark_file_path.empty()) {
        auto dark_val_opt = ProcessDarkFrame(opts.dark_file_path, log_stream);
        if (!dark_val_opt) { 
            log_stream << _("Fatal error processing dark frame.") << std::endl; 
            return false;
        }
        opts.dark_value = *dark_val_opt;
    }
    if (!opts.sat_file_path.empty()) {
        auto sat_val_opt = ProcessSaturationFrame(opts.sat_file_path, log_stream);
        if (!sat_val_opt) { 
            log_stream << _("Fatal error processing saturation frame.") << std::endl; 
            return false;
        }
        opts.saturation_value = *sat_val_opt;
    }
    
    return true;
}

} // namespace DynaRange::Engine::Initialization