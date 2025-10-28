// File: src/core/utils/OutputFilenameGenerator.cpp
/**
 * @file src/core/utils/OutputFilenameGenerator.cpp
 * @brief Implements the OutputFilenameGenerator class.
 */
#include "OutputFilenameGenerator.hpp"
#include "Constants.hpp"
#include "Formatters.hpp" // For DataSourceToString and GenerateChannelSuffix
#include <sstream>
#include <cmath>     // For std::round
#include <algorithm> // For std::replace

// --- Private Helpers ---

std::string OutputFilenameGenerator::SanitizeForFilename(const std::string& input) {
    std::string output = input;
    // Replace spaces with underscores
    std::replace(output.begin(), output.end(), ' ', '_');
    // Future: Add more replacements here if needed (e.g., '/', '\', ':')
    return output;
}

std::string OutputFilenameGenerator::GetSafeCameraSuffix(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    // Use the effective name directly from the context
    const std::string& effective_name = ctx.effective_camera_name_for_output;
    if (!effective_name.empty()) {
        // Use separator and sanitize the provided effective name
        return FNAME_SEPARATOR + SanitizeForFilename(effective_name);
    }
    // Return empty string if no effective name was provided
    return "";
}

std::string OutputFilenameGenerator::GetChannelSuffix(const OutputNamingContext& ctx) {
    // Simply call the existing Formatter function
    return Formatters::GenerateChannelSuffix(ctx.raw_channels);
}

std::string OutputFilenameGenerator::GetPlotFormatExtension(DynaRange::Graphics::Constants::PlotOutputFormat format) {
    // Use constants defined in Constants.hpp
    using namespace DynaRange::Graphics::Constants;
    using namespace DynaRange::Utils::Constants;
    switch (format) {
        case PlotOutputFormat::SVG: return EXT_SVG;
        case PlotOutputFormat::PDF: return EXT_PDF;
        case PlotOutputFormat::PNG: // Fallthrough intended
        default:                    return EXT_PNG; // Default to PNG
    }
}

// --- Public Static Methods ---

fs::path OutputFilenameGenerator::GenerateSummaryPlotFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    std::stringstream ss;
    ss << FNAME_BASE_SNR_CURVES                  // 1. "snr_curves"
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name
       << GetChannelSuffix(ctx);               // 3-6. Channel suffixes handled by Formatter
    ss << GetPlotFormatExtension(ctx.plot_format); // 7. Extension
    return ss.str();
}

fs::path OutputFilenameGenerator::GenerateIndividualPlotFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    std::stringstream ss;
    ss << FNAME_BASE_SNR_CURVE;                 // 1. "snr_curve"
    if (ctx.iso_speed.has_value()) {
        ss << FNAME_SEPARATOR << FNAME_ISO_PREFIX // 2. "_ISOxxx"
           << static_cast<int>(std::round(*ctx.iso_speed));
    }
    ss << GetSafeCameraSuffix(ctx)             // 3. Optional camera name
       << GetChannelSuffix(ctx);               // 4-7. Channel suffixes handled by Formatter
    ss << GetPlotFormatExtension(ctx.plot_format); // 8. Extension
    return ss.str();
}

fs::path OutputFilenameGenerator::GenerateCsvFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    // B3: Check for user override first
    if (!ctx.user_csv_filename.empty()) {
        return ctx.user_csv_filename;
    }
    // Default generation
    std::stringstream ss;
    ss << FNAME_BASE_CSV_RESULTS               // 1. "results"
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name
       << EXT_CSV;                             // 3. ".csv"
    return ss.str();
}

fs::path OutputFilenameGenerator::GeneratePrintPatchesFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    // B4: Check for user override first
    if (!ctx.user_print_patches_filename.empty() && ctx.user_print_patches_filename != "_USE_DEFAULT_PRINT_PATCHES_") {
        return ctx.user_print_patches_filename;
    }
    // Default generation
    std::stringstream ss;
    ss << FNAME_BASE_PRINT_PATCHES             // 1. "printpatches"
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name
       << EXT_PNG;                             // 3. ".png"
    return ss.str();
}

fs::path OutputFilenameGenerator::GenerateTestChartFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    // B5: Default generation
    std::stringstream ss;
    ss << FNAME_BASE_TEST_CHART                // 1. "testchart"
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name
       << EXT_PNG;                             // 3. ".png"
    return ss.str();
}

fs::path OutputFilenameGenerator::GenerateCornerDebugFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    // B6: Default generation
    std::stringstream ss;
    ss << FNAME_BASE_CORNER_DEBUG              // 1. "debug_corners_detected"
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name
       << EXT_PNG;                             // 3. ".png"
    return ss.str();
}

fs::path OutputFilenameGenerator::GeneratePreKeystoneDebugFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    std::stringstream ss;
    ss << "debug_pre_keystone"                 // 1. Base name
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name (using EXIF for debug)
       << EXT_PNG;                             // 3. ".png"
    return ss.str();
}

fs::path OutputFilenameGenerator::GeneratePostKeystoneDebugFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    std::stringstream ss;
    ss << "debug_post_keystone"                // 1. Base name
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name (using EXIF for debug)
       << EXT_PNG;                             // 3. ".png"
    return ss.str();
}

fs::path OutputFilenameGenerator::GenerateCropAreaDebugFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    std::stringstream ss;
    ss << "debug_crop_area"                    // 1. Base name
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name (using EXIF for debug)
       << EXT_PNG;                             // 3. ".png"
    return ss.str();
}

fs::path OutputFilenameGenerator::GenerateCornersDebugFilename(const OutputNamingContext& ctx) {
    using namespace DynaRange::Utils::Constants;
    std::stringstream ss;
    ss << "debug_corners"                    // 1. Base name
       << GetSafeCameraSuffix(ctx)             // 2. Optional camera name (using EXIF for debug)
       << EXT_PNG;                             // 3. ".png"
    return ss.str();
}