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

/**
 * @struct FlatResultRow
 * @brief Represents a single, flattened row of data for final output.
 */
struct FlatResultRow {
    std::string filename;
    double snr_threshold_db;
    float iso_speed;
    double dr_ev;
    DataSource channel;
    int samples_R;
    int samples_G1;
    int samples_G2;
    int samples_B;
};

std::string DataSourceToString(DataSource channel);

/**
 * @brief Flattens and sorts the hierarchical analysis results into a simple list of rows.
 * @details The sorting is performed based on three keys in order:
 * 1. SNR threshold (descending)
 * 2. ISO speed (ascending)
 * 3. Filename (ascending)
 * @param all_results The vector of hierarchical DynamicRangeResult structs.
 * @return A sorted vector of FlatResultRow structs, ready for display or saving.
 */
std::vector<FlatResultRow> FlattenAndSortResults(const std::vector<DynamicRangeResult>& all_results);
std::string FormatResultsTable(const std::vector<FlatResultRow>& sorted_rows);

/**
 * @brief Formats the CSV header string according to the new "long" format.
 * @return A string containing the CSV header row.
 */
std::string FormatCsvHeader();
/**
 * @brief Formats a single flattened result row into a CSV string.
 * @param row The FlatResultRow to format.
 * @return A string containing a single CSV row, ending with a newline.
 */
std::string FormatCsvRow(const FlatResultRow& row);
/**
 * @brief Generates a filename suffix based on the selected RAW channels.
 * @param channels The selection state of the RAW channels.
 * @return A string like "_average", "_channels_R_G1", or "_average_channels_R_G1".
 */
std::string GenerateChannelSuffix(const RawChannelSelection& channels);

} // namespace Formatters