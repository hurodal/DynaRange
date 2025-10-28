// File: src/core/artifacts/ArtifactFactory.hpp
/**
 * @file src/core/artifacts/ArtifactFactory.hpp
 * @brief Declares the ArtifactFactory responsible for creating and saving output artifacts.
 * @details This factory centralizes the logic for generating filenames, titles (if applicable),
 * content, and writing various output files (CSV, plots, debug images, logs) to disk.
 * It uses other core modules (generators, writers, path manager) to perform its tasks.
 */
#pragma once

#include "../engine/Reporting.hpp"            // For ReportingParameters
#include "../utils/OutputNamingContext.hpp"   // For OutputNamingContext
#include "../utils/PathManager.hpp"           // For PathManager
#include "../arguments/ChartOptionsParser.hpp" // For ChartGeneratorOptions
#include <opencv2/core.hpp>                 // For cv::Mat
#include <filesystem>
#include <optional>
#include <ostream>
#include <vector>

namespace fs = std::filesystem;
/**
 * @struct InMemoryImage
 * @brief A generic, library-agnostic container for raw image data (e.g., thumbnails).
 */
struct InMemoryImage {
    std::vector<unsigned char> data; ///< Raw pixel data in RGB format.
    int width;                       ///< Image width in pixels.
    int height;                      ///< Image height in pixels.
};
namespace ArtifactFactory {

/**
 * @brief Creates and saves the generated test chart image.
 * @param chart_opts Parameters defining the chart to generate.
 * @param ctx The context for generating the filename.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved chart file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateTestChartImage(
    const ChartGeneratorOptions& chart_opts,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream);

/**
 * @brief Generates a small, in-memory thumbnail of a test chart.
 * @param opts A struct containing all validated chart parameters.
 * @param thumb_width The desired width of the thumbnail in pixels.
 * @return An optional containing the generated thumbnail data, or nullopt on failure.
 */
std::optional<InMemoryImage> GenerateChartThumbnail(const ChartGeneratorOptions& opts, int thumb_width);
} // namespace ArtifactFactory