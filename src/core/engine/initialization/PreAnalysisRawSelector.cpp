// File: src/core/engine/initialization/PreAnalysisRawSelector.cpp
/**
 * @file PreAnalysisRawSelector.cpp
 * @brief Implements the source RAW file selection logic for the pre-analysis phase.
 */
#include "PreAnalysisRawSelector.hpp"
#include "../setup/Constants.hpp"
#include <opencv2/core.hpp>
#include <filesystem>
#include <libintl.h>

#define _(string) gettext(string)
namespace fs = std::filesystem;

namespace DynaRange::Engine::Initialization {

int SelectPreAnalysisRawIndex(
    const std::vector<PreAnalysisResult>& sorted_pre_analysis_results, // <<-- PARÁMETRO CAMBIADO
    std::ostream& log_stream)
{
    int source_image_index = 0; // Default to the darkest file (index 0 of sorted list)
    bool found_suitable = false;

    // Iterate backwards from the brightest file (end of the list)
    // to find the first one that is NOT marked as saturated.
    for (int i = sorted_pre_analysis_results.size() - 1; i >= 0; --i) {
        if (!sorted_pre_analysis_results[i].has_saturated_pixels) {
            source_image_index = i; // Found the brightest non-saturated file
            found_suitable = true;
            break;
        }
    }

    // Log the selection based on the result
    const std::string& selected_filename = sorted_pre_analysis_results.empty() ?
                                            "N/A" : // Handle empty list case
                                            sorted_pre_analysis_results[source_image_index].filename;

    if (found_suitable) {
        log_stream << _("[INFO] Selected '") << fs::path(selected_filename).filename().string()
                   << _("' as the source image for detection (brightest with < ")
                   << (DynaRange::Setup::Constants::MAX_PRE_ANALYSIS_SATURATION_RATIO * 100.0)
                   << _("% saturated pixels).") << std::endl;
    } else {
        log_stream << _("[INFO] All images contain saturated pixels (>= ")
                   << (DynaRange::Setup::Constants::MAX_PRE_ANALYSIS_SATURATION_RATIO * 100.0)
                   << _("%). Selected '") << fs::path(selected_filename).filename().string()
                   << _("' as the source image for detection (darkest).") << std::endl;
    }

    return source_image_index;
}

} // namespace DynaRange::Engine::Initialization