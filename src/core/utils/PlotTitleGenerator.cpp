// File: src/core/utils/PlotTitleGenerator.cpp
/**
 * @file src/core/utils/PlotTitleGenerator.cpp
 * @brief Implements the PlotTitleGenerator class.
 */
#include "PlotTitleGenerator.hpp"
#include "Constants.hpp"
#include <sstream>
#include <cmath>
#include <libintl.h>

#define _(string) gettext(string)

std::string PlotTitleGenerator::GenerateSummaryTitle(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    // Use the effective name provided by the calling layer
    const std::string& camera_name = ctx.effective_camera_name_for_output;
    std::stringstream ss;

    ss << _(TITLE_BASE_SNR_CURVES);

    // Append camera name in parentheses if an effective name was provided
    if (!camera_name.empty()) {
        ss << " (" << camera_name << ")";
    }
    return ss.str();
}

std::string PlotTitleGenerator::GenerateIndividualTitle(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    // Use the effective name provided by the calling layer
    const std::string& camera_name = ctx.effective_camera_name_for_output;
    std::stringstream ss;

    // ISO speed is essential for an individual title
    if (!ctx.iso_speed.has_value()) {
        return ""; // Cannot generate title without ISO
    }

    ss << _(TITLE_BASE_SNR_CURVE);

    // Add parentheses if either camera name or ISO is present (ISO is guaranteed here)
    if (!camera_name.empty() || ctx.iso_speed.has_value()) {
        ss << " (";
        bool need_comma = false;

        // Add effective camera name if available
        if (!camera_name.empty()) {
            ss << camera_name;
            need_comma = true; // Need comma before adding ISO
        }

        // Add formatted ISO speed (guaranteed present)
        if (need_comma) {
            ss << ", ";
        }
        ss << FNAME_ISO_PREFIX << static_cast<int>(std::round(*ctx.iso_speed));

        ss << ")"; // Close parenthesis
    }
    // If only ISO was present (no effective camera name), title is like "SNR Curve (ISO 200)"
    return ss.str();
}