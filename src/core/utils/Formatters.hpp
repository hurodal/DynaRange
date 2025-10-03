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
std::string FormatCsvHeader(const ProgramOptions& opts);
std::string FormatCsvRow(const DynamicRangeResult& res, const ProgramOptions& opts);
/**
 * @brief Generates a filename suffix based on the selected RAW channels.
 * @param channels The selection state of the RAW channels.
 * @return A string like "_average", "_channels_R_G1", or "_average_channels_R_G1".
 */
std::string GenerateChannelSuffix(const RawChannelSelection& channels);

} // namespace Formatters