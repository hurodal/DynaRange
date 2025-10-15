// File: src/core/engine/initialization/PreAnalysisRawSelector.cpp
/**
 * @file PreAnalysisRawSelector.cpp
 * @brief Implements the source RAW file selection logic for the pre-analysis phase.
 */
#include "PreAnalysisRawSelector.hpp"
#include <opencv2/core.hpp>
#include <filesystem>
#include <libintl.h>

#define _(string) gettext(string)
namespace fs = std::filesystem;

namespace DynaRange::Engine::Initialization {

int SelectPreAnalysisRawIndex(
    const std::vector<RawFile>& sorted_files,
    double saturation_value,
    std::ostream& log_stream)
{
    int source_image_index = 0; // Default to the darkest file (index 0 of sorted list)
    bool found_non_saturated = false;
    
    // Iterate backwards from the brightest file (end of the list) to find the first non-saturated one.
    for (int i = sorted_files.size() - 1; i >= 0; --i) {
        const auto& raw_file = sorted_files[i];
        if (raw_file.IsLoaded()) {
            cv::Mat active_img = raw_file.GetActiveRawImage();
            if (!active_img.empty()) {
                // A pixel is considered saturated if it's at 99% or more of the saturation level.
                int saturated_pixels = cv::countNonZero(active_img >= (saturation_value * 0.99));
                if (saturated_pixels == 0) {
                    source_image_index = i; // Found the brightest non-saturated file
                    found_non_saturated = true;
                    break;
                }
            }
        }
    }

    if (found_non_saturated) {
        log_stream << _("[INFO] Selected '") << fs::path(sorted_files[source_image_index].GetFilename()).filename().string()
                   << _("' as the source image for detection (brightest non-saturated).") << std::endl;
    } else {
        log_stream << _("[INFO] All images contain saturated pixels. Selected '") << fs::path(sorted_files[source_image_index].GetFilename()).filename().string()
                   << _("' as the source image for detection (darkest).") << std::endl;
    }
    
    return source_image_index;
}

} // namespace DynaRange::Engine::Initialization