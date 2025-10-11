// File: src/core/utils/Formatters.cpp
/**
 * @file src/core/utils/Formatters.cpp
 * @brief Implements the data formatting utility functions.
 */
#include "Formatters.hpp"
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <libintl.h>
#include <sstream>
#include <string>
#include <vector>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace Formatters {

std::string DataSourceToString(DataSource channel) {
    switch (channel) {
        case DataSource::R:   return "R";
        case DataSource::G1:  return "G1";
        case DataSource::G2:  return "G2";
        case DataSource::B:   return "B";
        case DataSource::AVG: return "AVG";
    }
    return "Unknown";
}

std::string GenerateChannelSuffix(const RawChannelSelection& channels) {
    std::string suffix;
    if (channels.AVG) {
        suffix += "_average";
    }

    std::vector<std::string> selected_channels;
    if (channels.R) selected_channels.push_back("R");
    if (channels.G1) selected_channels.push_back("G1");
    if (channels.G2) selected_channels.push_back("G2");
    if (channels.B) selected_channels.push_back("B");

    if (!selected_channels.empty()) {
        suffix += "_channels";
        for(const auto& ch : selected_channels) {
            suffix += "_" + ch;
        }
    }
    return suffix;
}

std::vector<FlatResultRow> FlattenAndSortResults(const std::vector<DynamicRangeResult>& all_results) {
    std::vector<FlatResultRow> flat_rows;
    // 1. Flatten the data from the hierarchical structure to a simple list of rows.
    for (const auto& res : all_results) {
        for (const auto& pair : res.dr_values_ev) {
            flat_rows.push_back({
                res.filename,
                pair.first, // snr_threshold_db
                res.channel,
                res.iso_speed,
                res.samples_R,
                res.samples_G1,
                res.samples_G2,
                res.samples_B,
                pair.second // dr_ev
            });
        }
    }

    // 2. Sort the flat list using the specified multi-key criteria.
    std::sort(flat_rows.begin(), flat_rows.end(), [](const FlatResultRow& a, const FlatResultRow& b) {
        if (a.snr_threshold_db != b.snr_threshold_db) {
            return a.snr_threshold_db > b.snr_threshold_db; // Descending threshold
        }
        if (a.iso_speed != b.iso_speed) {
            return a.iso_speed < b.iso_speed; // Ascending ISO
        }
        return a.filename < b.filename; // Ascending filename
    });

    return flat_rows;
}

std::string FormatResultsTable(const std::vector<FlatResultRow>& sorted_rows) {
    if (sorted_rows.empty()) {
        return "";
    }

    // --- Define Headers and initial widths ---
    const std::vector<std::string> headers = {
        _("raw_file"), _("SNR_db"), _("ISO"), _("Channel"), 
        "samples_R", "samples_G1", "samples_G2", "samples_B", "DR_EV"
    };
    std::vector<size_t> widths;
    for(const auto& h : headers) {
        widths.push_back(h.length());
    }

    // --- Measure max widths from data ---
    for (const auto& row : sorted_rows) {
        widths[0] = std::max(widths[0], fs::path(row.filename).filename().string().length());
        
        std::stringstream ss_thresh, ss_iso, ss_dr;
        ss_thresh << std::fixed << std::setprecision(2) << row.snr_threshold_db;
        ss_iso << static_cast<int>(row.iso_speed);
        ss_dr << std::fixed << std::setprecision(4) << row.dr_ev;

        widths[1] = std::max(widths[1], ss_thresh.str().length());
        widths[2] = std::max(widths[2], ss_iso.str().length());
        widths[3] = std::max(widths[3], DataSourceToString(row.channel).length());
        widths[4] = std::max(widths[4], std::to_string(row.samples_R).length());
        widths[5] = std::max(widths[5], std::to_string(row.samples_G1).length());
        widths[6] = std::max(widths[6], std::to_string(row.samples_G2).length());
        widths[7] = std::max(widths[7], std::to_string(row.samples_B).length());
        widths[8] = std::max(widths[8], ss_dr.str().length());
    }

    // --- Build Table ---
    auto add_padding = [](size_t& width) { width += 2; };
    for(auto& w : widths) add_padding(w);

    std::stringstream table_ss;
    table_ss << std::left << std::setw(widths[0]) << headers[0]
             << std::right << std::setw(widths[1]) << headers[1]
             << std::right << std::setw(widths[2]) << headers[2]
             << std::left << std::setw(widths[3]) << headers[3]
             << std::right << std::setw(widths[4]) << headers[4]
             << std::right << std::setw(widths[5]) << headers[5]
             << std::right << std::setw(widths[6]) << headers[6]
             << std::right << std::setw(widths[7]) << headers[7]
             << std::right << std::setw(widths[8]) << headers[8] << "\n";
             
    size_t total_width = 0;
    for(auto w : widths) total_width += w;
    table_ss << std::string(total_width, '-') << "\n";
    
    // Data rows from sorted list
    for (const auto& row : sorted_rows) {
        table_ss << std::left << std::setw(widths[0]) << fs::path(row.filename).filename().string()
                 << std::right << std::setw(widths[1]) << std::fixed << std::setprecision(2) << row.snr_threshold_db
                 << std::right << std::setw(widths[2]) << static_cast<int>(row.iso_speed)
                 << std::left << std::setw(widths[3]) << DataSourceToString(row.channel)
                 << std::right << std::setw(widths[4]) << row.samples_R
                 << std::right << std::setw(widths[5]) << row.samples_G1
                 << std::right << std::setw(widths[6]) << row.samples_G2
                 << std::right << std::setw(widths[7]) << row.samples_B
                 << std::right << std::setw(widths[8]) << std::fixed << std::setprecision(4) << row.dr_ev << "\n";
    }
    return table_ss.str();
}

std::string FormatCsvHeader() {
    return "raw_file,SNRthreshold_db,ISO,raw_channel,samples_R,samples_G1,samples_G2,samples_B,DR_EV";
}

std::string FormatCsvRow(const FlatResultRow& row) {
    std::stringstream row_ss;
    row_ss << fs::path(row.filename).filename().string() << ","
           << std::fixed << std::setprecision(2) << row.snr_threshold_db << ","
           << static_cast<int>(row.iso_speed) << ","
           << DataSourceToString(row.channel) << ","
           << row.samples_R << "," << row.samples_G1 << "," 
           << row.samples_G2 << "," << row.samples_B << ","
           << std::fixed << std::setprecision(4) << row.dr_ev << "\n";
    return row_ss.str();
}
} // namespace Formatters