// File: src/core/artifacts/data/ReportWriter.hpp
/**
 * @file src/core/artifacts/data/ReportWriter.hpp
 * @brief Declares functions for generating and saving data and log file artifacts.
 * @details This module encapsulates the logic for creating and writing structured reports (CSV) and logs (TXT).
 */
#pragma once

#include "../../analysis/Analysis.hpp"
#include "../../utils/OutputNamingContext.hpp"
#include "../../utils/PathManager.hpp"
#include <filesystem>
#include <optional>
#include <ostream>
#include <vector>

namespace fs = std::filesystem;

namespace ArtifactFactory::Report {

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

} // namespace ArtifactFactory::Report