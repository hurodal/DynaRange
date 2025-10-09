// File: src/core/utils/Formatters.hpp
/**
 * @file src/core/utils/Formatters.hpp
 * @brief Declares utility functions for formatting data into strings.
 */
#pragma once

#include "../analysis/Analysis.hpp"
#include "../arguments/ArgumentsOptions.hpp"
#include <string>
#include <vector>

namespace Formatters {

std::string DataSourceToString(DataSource channel);
std::string FormatResultsTable(const std::vector<DynamicRangeResult>& all_results, const ProgramOptions& opts);

/**
 * @brief Formats the CSV header string according to the new "long" format.
 * @return A string containing the CSV header row.
 */
std::string FormatCsvHeader();

/**
 * @brief Formats all the CSV rows for a single DynamicRangeResult.
 * @details This function iterates through all calculated DR values for a given
 * result and creates a separate CSV row for each one.
 * @param res The DynamicRangeResult to format.
 * @return A string containing one or more CSV rows, each ending with a newline.
 */
std::string FormatCsvRows(const DynamicRangeResult& res);

/**
 * @brief Generates a filename suffix based on the selected RAW channels.
 * @param channels The selection state of the RAW channels.
 * @return A string like "_average", "_channels_R_G1", or "_average_channels_R_G1".
 */
std::string GenerateChannelSuffix(const RawChannelSelection& channels);

} // namespace Formatters