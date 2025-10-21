// File: src/core/utils/OutputFilenameGenerator.hpp
/**
 * @file src/core/utils/OutputFilenameGenerator.hpp
 * @brief Declares a class responsible for generating standardized output filenames.
 */
#pragma once

#include "OutputNamingContext.hpp"
#include "../graphics/Constants.hpp"
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class OutputFilenameGenerator {
public:
    // --- Public static methods for filename generation ---
    /** B1: Generates the filename for the summary plot. */
    static fs::path GenerateSummaryPlotFilename(const OutputNamingContext& ctx);
    /** B2: Generates the filename for an individual ISO plot. */
    static fs::path GenerateIndividualPlotFilename(const OutputNamingContext& ctx);
    /** B3: Generates the filename for the CSV results file. */
    static fs::path GenerateCsvFilename(const OutputNamingContext& ctx);
    /** B4: Generates the filename for the debug patches image. */
    static fs::path GeneratePrintPatchesFilename(const OutputNamingContext& ctx);
    /** B5: Generates the filename for the generated test chart image. */
    static fs::path GenerateTestChartFilename(const OutputNamingContext& ctx);
    /** B6: Generates the filename for the corner detection debug image. */
    static fs::path GenerateCornerDebugFilename(const OutputNamingContext& ctx);

    /** B7: Generates the filename for the pre-keystone debug image. */
    static fs::path GeneratePreKeystoneDebugFilename(const OutputNamingContext& ctx);
    /** B8: Generates the filename for the post-keystone debug image. */
    static fs::path GeneratePostKeystoneDebugFilename(const OutputNamingContext& ctx);
    /** B9: Generates the filename for the crop area debug image. */
    static fs::path GenerateCropAreaDebugFilename(const OutputNamingContext& ctx);

    /**
     * @brief Internal helper to get the sanitized camera name suffix part.
     * @param ctx The context providing the effective_camera_name_for_output.
     * @return The camera suffix string (e.g., "_OM-1") or an empty string if no effective name provided.
     */
    static std::string GetSafeCameraSuffix(const OutputNamingContext& ctx);
private:
    // --- Private static helper methods ---
    /** @brief Internal helper to get the channel suffix part (delegates to Formatters).*/
    static std::string GetChannelSuffix(const OutputNamingContext& ctx);
    /** @brief Internal helper to get the file extension based on PlotOutputFormat. */
    static std::string GetPlotFormatExtension(DynaRange::Graphics::Constants::PlotOutputFormat format);
    /** @brief Helper to replace spaces with underscores for filenames. */
    static std::string SanitizeForFilename(const std::string& input);
};