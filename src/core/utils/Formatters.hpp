// File: src/core/utils/Formatters.hpp
/**
 * @file src/core/utils/Formatters.hpp
 * @brief Declares utility functions for formatting data into strings.
 * @details This module adheres to SRP by centralizing all data-to-string
 * formatting logic, separating it from the reporting and I/O modules.
 */
#pragma once

#include "../analysis/Analysis.hpp"
#include "../arguments/ArgumentsOptions.hpp"
#include <string>
#include <vector>

namespace Formatters {

/**
 * @brief Formats the entire results table for console log output with dynamic column widths.
 * @param all_results The vector of all DynamicRangeResult structs.
 * @param opts The program options, used to get SNR thresholds.
 * @return A single string containing the fully formatted table.
 */
std::string FormatResultsTable(const std::vector<DynamicRangeResult>& all_results, const ProgramOptions& opts);

/**
 * @brief Formats a header row for a CSV file.
 * @param opts The program options.
 * @return A string containing the formatted header row.
 */
std::string FormatCsvHeader(const ProgramOptions& opts);

/**
 * @brief Formats a single data row for a CSV file.
 * @param res The DynamicRangeResult for the row.
 * @param opts The program options.
 * @return A string containing the formatted data row.
 */
std::string FormatCsvRow(const DynamicRangeResult& res, const ProgramOptions& opts);

} // namespace Formatters