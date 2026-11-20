// File: src/core/engine/initialization/CalibrationHandler.cpp
/**
 * @file CalibrationHandler.cpp
 * @brief Implements the sensor calibration logic.
 */
#include "CalibrationHandler.hpp"
#include "../../analysis/RawProcessor.hpp"
#include "../../setup/CalibrationEstimator.hpp"
#include "../../io/raw/RawFile.hpp"
#include <libintl.h>
#include <optional>
#include <string>

#define _(string) gettext(string)

namespace DynaRange::Engine::Initialization {

namespace {
/**
 * @brief Reads the black level stored in the RAW metadata (LibRaw).
 * @details Uses the dark frame if one was given, otherwise the first input file.
 */
std::optional<double> ReadMetadataBlackLevel(const ProgramOptions& opts, const std::vector<FileInfo>& file_info)
{
    std::string source = opts.dark_file_path;
    if (source.empty() && !file_info.empty()) source = file_info.front().filename;
    if (source.empty()) return std::nullopt;
    RawFile raw_file(source);
    if (!raw_file.Load()) return std::nullopt;
    return raw_file.GetBlackLevelFromMetadata();
}
} // namespace

bool CalibrationHandler::HandleCalibration(ProgramOptions& opts, const std::vector<FileInfo>& file_info, std::ostream& log_stream) const
{
    // --- 0. BLACK LEVEL STORED IN THE RAW METADATA ---
    // Some cameras (e.g. Nikon D600/D610/D800) subtract the black level in-camera and clip
    // the result at 0. Their metadata black level is 0 and about half of the dark noise is
    // stored as exact zeros. For them the black level must be 0: the mean of a dark frame
    // would be biased upwards and the minimum-value heuristic falls back to 256.
    // With black = 0 the patch analysis reconstructs the clipped noise (censored normal).
    const std::optional<double> metadata_black = ReadMetadataBlackLevel(opts, file_info);
    bool black_resolved = false;
    // Only an explicit numeric level (-B without -b) overrides this; note that giving a
    // dark frame (-b) also clears black_level_is_default.
    const bool explicit_numeric_black = !opts.black_level_is_default && opts.dark_file_path.empty();
    if (metadata_black && *metadata_black == 0.0 && !explicit_numeric_black) {
        opts.dark_value = 0.0;
        black_resolved = true;
        log_stream << _("[INFO] RAW metadata reports a black level of 0: the camera clips black in-camera. Using black level 0 and reconstructing clipped shadow noise.") << std::endl;
        if (!opts.dark_file_path.empty()) {
            log_stream << _("  - The dark frame is not used for the black level: its mean is biased by the clipping at 0.") << std::endl;
        }
    }

    // --- 1. DEFAULT CALIBRATION ESTIMATION ---
    if (!black_resolved && opts.dark_file_path.empty() && opts.black_level_is_default) {
        if (metadata_black && *metadata_black > 0.0) {
            opts.dark_value = *metadata_black;
            log_stream << _("[INFO] Black level not specified. Using the black level stored in the RAW metadata: ")
                       << opts.dark_value << std::endl;
            log_stream << _("  - NOTE: For maximum accuracy, providing a dedicated dark frame (-b) is recommended.") << std::endl;
        } else {
            log_stream << _("[INFO] Black level not specified. Attempting to estimate from RAW file...") << std::endl;
            auto estimated_black = CalibrationEstimator::EstimateBlackLevel(opts, file_info, log_stream);
            if (estimated_black) {
                opts.dark_value = *estimated_black;
            } else {
                log_stream << _("[Warning] Could not estimate black level. Using fallback default value: ")
                           << opts.dark_value << std::endl;
            }
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
    if (!black_resolved && !opts.dark_file_path.empty()) {
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