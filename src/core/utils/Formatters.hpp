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

} // namespace Formatters