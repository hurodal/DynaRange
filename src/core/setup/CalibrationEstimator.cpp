// File: src/core/setup/CalibrationEstimator.cpp
/**
 * @file CalibrationEstimator.cpp
 * @brief Implements the calibration value estimation logic.
 */
#include "CalibrationEstimator.hpp"
#include "../io/RawFile.hpp"
#include "../arguments/ArgumentsOptions.hpp" // For DEFAULT_BLACK_LEVEL
#include <libintl.h>
#include <filesystem>
#include <cmath>
#include <opencv2/core.hpp> // For cv::minMaxLoc
#include <algorithm> // For std::min_element, std::max_element

#define _(string) gettext(string)
namespace fs = std::filesystem;

namespace CalibrationEstimator {

std::optional<double> EstimateBlackLevel(const ProgramOptions& opts, const std::vector<FileInfo>& file_info, std::ostream& log_stream) {
    if (file_info.empty()) {
        return std::nullopt;
    }

    // Find the file with the lowest mean brightness (the darkest file).
    auto darkest_file_it = std::min_element(file_info.begin(), file_info.end(), 
        [](const FileInfo& a, const FileInfo& b) {
            return a.mean_brightness < b.mean_brightness;
        });

    const std::string& estimation_file = darkest_file_it->filename;
    log_stream << _("  - Selecting '") << fs::path(estimation_file).filename().string() 
               << _("' for estimation (it is the darkest image).") << std::endl;

    RawFile raw_file(estimation_file);
    if (!raw_file.Load()) {
        log_stream << _("  - [Warning] Could not open RAW file to estimate black level: ") << estimation_file << std::endl;
        return std::nullopt;
    }

    cv::Mat active_img = raw_file.GetActiveRawImage();
    if (active_img.empty()) {
        log_stream << _("  - [Warning] Could not get active image area to estimate black level.") << std::endl;
        return std::nullopt;
    }

    double min_val, max_val;
    cv::minMaxLoc(active_img, &min_val, &max_val);

    if (min_val <= 1.0) {
        log_stream << _("  - [Warning] Minimum pixel value is too low to reliably estimate black level. Using fallback.") << std::endl;
        return DEFAULT_BLACK_LEVEL;
    }

    // Calculate the nearest power of 2 to the minimum value found by using std::round.
    double exponent = std::round(std::log2(min_val));
    double estimated_black = std::pow(2.0, exponent);

    log_stream << _("  - Estimated black level as the nearest power of 2 to the minimum pixel value.") << std::endl;
    log_stream << _("  - NOTE: For maximum accuracy, providing a dedicated dark frame (-B) is recommended.") << std::endl;
    log_stream << _("  - Minimum pixel value found: ") << min_val << _(". Estimated black level: ") << estimated_black << std::endl;
    
    return estimated_black;
}

std::optional<double> EstimateSaturationLevel(const ProgramOptions& opts, const std::vector<FileInfo>& file_info, std::ostream& log_stream) {
    if (file_info.empty()) {
        return std::nullopt;
    }

    // Find the file with the highest ISO speed from the pre-analyzed list.
    auto highest_iso_file_it = std::max_element(file_info.begin(), file_info.end(),
        [](const FileInfo& a, const FileInfo& b) {
            return a.iso_speed < b.iso_speed;
        });

    const std::string& estimation_file = highest_iso_file_it->filename;

    RawFile raw_file(estimation_file);
    if (!raw_file.Load()) {
        log_stream << _("  - [Warning] Could not open RAW file to estimate saturation level: ") << estimation_file << std::endl;
        return std::nullopt;
    }
    
    std::optional<int> bit_depth_opt = raw_file.GetBitDepth();
    int bit_depth;

    if (bit_depth_opt.has_value()) {
        bit_depth = *bit_depth_opt;
        log_stream << _("  - Estimated from '")
                   << fs::path(estimation_file).filename().string() << "' (Highest ISO file, "
                   << bit_depth << " bits): ";
    } else {
        bit_depth = 14; // Fallback value
        log_stream << _("  - [Warning] Could not determine bit depth from RAW metadata. Using a default fallback of ")
                   << bit_depth << _(" bits. This value may not be accurate for your camera.") << std::endl;
        log_stream << _("  - Estimated saturation level: ");
    }

    double sat_level = static_cast<double>((1 << bit_depth) - 1);
    log_stream << sat_level << std::endl;

    return sat_level;
}

} // namespace CalibrationEstimator