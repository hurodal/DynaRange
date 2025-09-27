// File: src/core/utils/Formatters.hpp
/**
 * @file src/core/utils/Formatters.hpp
 * @brief Declares utility functions for formatting data into strings.
 * @details This module adheres to SRP by centralizing all data-to-string
 * formatting logic, separating it from the reporting and I/O modules.
 */
#pragma once

#include "../analysis/Analysis.hpp"
#include "../arguments/ProgramOptions.hpp"
#include <string>

namespace Formatters {

/**
 * @enum FormatType
 * @brief Specifies the target format for the string output.
 */
enum class FormatType {
    Log, ///< Format for fixed-width console log output.
    Csv  ///< Format for comma-separated value file output.
};

/**
 * @brief Formats the header for the results table.
 * @param opts The program options, used to get SNR thresholds.
 * @param type The target format (Log or Csv).
 * @return A string containing the formatted header row.
 */
std::string FormatResultHeader(const ProgramOptions& opts, FormatType type);

/**
 * @brief Formats a single result data row for the results table.
 * @param res The DynamicRangeResult for the row.
 * @param opts The program options, used to get SNR thresholds.
 * @param type The target format (Log or Csv).
 * @return A string containing the formatted data row.
 */
std::string FormatResultRow(const DynamicRangeResult& res, const ProgramOptions& opts, FormatType type);

} // namespace Formatters