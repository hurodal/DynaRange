// File: src/core/artifacts/ArtifactFactory.hpp
/**
 * @file src/core/artifacts/ArtifactFactory.hpp
 * @brief Declares the ArtifactFactory responsible for creating and saving output artifacts.
 * @details This factory centralizes the logic for generating filenames, titles (if applicable),
 * content, and writing various output files (CSV, plots, debug images, logs) to disk.
 * It uses other core modules (generators, writers, path manager) to perform its tasks.
 */
#pragma once

#include "../analysis/Analysis.hpp"           // For CurveData, DynamicRangeResult
#include "../engine/Reporting.hpp"            // For ReportingParameters
#include "../utils/OutputNamingContext.hpp"   // For OutputNamingContext
#include "../utils/PathManager.hpp"           // For PathManager
#include "../arguments/ChartOptionsParser.hpp" // For ChartGeneratorOptions
#include <opencv2/core.hpp>                 // For cv::Mat
#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
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
 * @brief Creates and saves the final CSV results file.
 * @param results The aggregated dynamic range results.
 * @param ctx The context for generating the filename.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved CSV file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateCsvReport(
    const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream);
/**
 * @brief Creates and saves the summary plot image.
 * @param curves The aggregated curve data for all files.
 * @param results The aggregated dynamic range results (for labels).
 * @param ctx The context for generating filename and title.
 * @param reporting_params Parameters controlling plot rendering details.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved plot file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateSummaryPlot(
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream);
/**
 * @brief Creates and saves an individual plot image for a single input file/ISO.
 * @param curves_for_file The curve data specific to this file.
 * @param results_for_file The dynamic range results specific to this file.
 * @param ctx The context for generating filename and title (must include ISO).
 * @param reporting_params Parameters controlling plot rendering details.
 * @param global_bounds Pre-calculated global axis boundaries for consistent scaling.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved plot file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateIndividualPlot(
    const std::vector<CurveData>& curves_for_file,
    const std::vector<DynamicRangeResult>& results_for_file,
    const OutputNamingContext& ctx,
    const ReportingParameters& reporting_params,
    const std::map<std::string, double>& global_bounds,
    const PathManager& paths,
    std::ostream& log_stream);
/**
 * @brief Creates and saves the debug image showing analyzed patches.
 * @param debug_image The OpenCV matrix (CV_32F, 0-1 range, gamma corrected) to save.
 * @param ctx The context for generating the filename.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved image file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreatePrintPatchesImage(
    const cv::Mat& debug_image,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream);
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
 * @brief Creates and saves a generic debug image (e.g., corners, pre/post keystone).
 * @details This function determines the correct filename using OutputFilenameGenerator based on
 * the type hinted by the OutputNamingContext (though currently relies on caller providing correct filename generator).
 * @param debug_image The OpenCV matrix (CV_32F, 0-1 range, gamma corrected) with markers/overlays.
 * @param filename The pre-generated, specific filename for this debug image.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved image file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateGenericDebugImage(
    const cv::Mat& debug_image,
    const fs::path& filename, // Takes pre-generated filename
    const PathManager& paths,
    std::ostream& log_stream);

/**
 * @brief Creates and saves the log output to a text file.
 * @param log_content The complete log content as a string.
 * @param ctx The context for generating the filename suffix.
 * @param base_output_directory The directory where the log should be saved (e.g., same as CSV).
 * @return An optional containing the full path to the saved log file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateLogFile(
    const std::string& log_content,
    const OutputNamingContext& ctx,
    const fs::path& base_output_directory);
/**
 * @brief Generates a small, in-memory thumbnail of a test chart.
 * @param opts A struct containing all validated chart parameters.
 * @param thumb_width The desired width of the thumbnail in pixels.
 * @return An optional containing the generated thumbnail data, or nullopt on failure.
 */
std::optional<InMemoryImage> GenerateChartThumbnail(const ChartGeneratorOptions& opts, int thumb_width);
} // namespace ArtifactFactory