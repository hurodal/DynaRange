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
    std::vector<std::string> individual_channels_selected;
    if (channels.R) individual_channels_selected.push_back("R");
    if (channels.G1) individual_channels_selected.push_back("G1");
    if (channels.G2) individual_channels_selected.push_back("G2");
    if (channels.B) individual_channels_selected.push_back("B");

    if (channels.avg_mode == AvgMode::Full) {
        suffix += "_average";
    } else if (channels.avg_mode == AvgMode::Selected) {
        suffix += "_average";
        for (const auto& ch : individual_channels_selected) {
            suffix += "_" + ch;
        }
    }

    if (!individual_channels_selected.empty()) {
        suffix += "_selected";
        for(const auto& ch : individual_channels_selected) {
            suffix += "_" + ch;
        }
    }
    
    return suffix;
}

std::vector<FlatResultRow> FlattenAndSortResults(const std::vector<DynamicRangeResult>& all_results) {
    std::vector<FlatResultRow> flat_rows;
    for (const auto& res : all_results) {
        for (const auto& pair : res.dr_values_ev) {
            flat_rows.push_back({
                res.filename,
                pair.first, // snr_threshold_db
                res.iso_speed,
                pair.second, // dr_ev
                res.channel,
                res.samples_R,
                res.samples_G1,
                res.samples_G2,
                res.samples_B
            });
        }
    }

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

    const std::vector<std::string> headers = {
        _("raw_file"), _("SNR_db"), _("ISO"), "DR_EV", _("Channel"), 
        "samples_R", "samples_G1", "samples_G2", "samples_B"
    };
    std::vector<size_t> widths;
    for(const auto& h : headers) {
        widths.push_back(h.length());
    }

    for (const auto& row : sorted_rows) {
        widths[0] = std::max(widths[0], fs::path(row.filename).filename().string().length());
        std::stringstream ss_thresh, ss_iso, ss_dr;
        ss_thresh << std::fixed << std::setprecision(2) << row.snr_threshold_db;
        ss_iso << static_cast<int>(row.iso_speed);
        ss_dr << std::fixed << std::setprecision(4) << row.dr_ev;

        widths[1] = std::max(widths[1], ss_thresh.str().length());
        widths[2] = std::max(widths[2], ss_iso.str().length());
        widths[3] = std::max(widths[3], ss_dr.str().length());
        widths[4] = std::max(widths[4], DataSourceToString(row.channel).length());
        widths[5] = std::max(widths[5], std::to_string(row.samples_R).length());
        widths[6] = std::max(widths[6], std::to_string(row.samples_G1).length());
        widths[7] = std::max(widths[7], std::to_string(row.samples_G2).length());
        widths[8] = std::max(widths[8], std::to_string(row.samples_B).length());
    }

    auto add_padding = [](size_t& width) { width += 2; };
    for(auto& w : widths) add_padding(w);

    std::stringstream table_ss;
    table_ss << std::left << std::setw(widths[0]) << headers[0]
             << std::right << std::setw(widths[1]) << headers[1]
             << std::right << std::setw(widths[2]) << headers[2]
             << std::right << std::setw(widths[3]) << headers[3]
             << std::right << std::setw(widths[4]) << headers[4]
             << std::right << std::setw(widths[5]) << headers[5]
             << std::right << std::setw(widths[6]) << headers[6]
             << std::right << std::setw(widths[7]) << headers[7]
             << std::right << std::setw(widths[8]) << headers[8] << "\n";
    size_t total_width = 0;
    for(auto w : widths) total_width += w;
    table_ss << std::string(total_width, '-') << "\n";
    
    for (const auto& row : sorted_rows) {
        table_ss << std::left << std::setw(widths[0]) << fs::path(row.filename).filename().string()
                 << std::right << std::setw(widths[1]) << std::fixed << std::setprecision(2) << row.snr_threshold_db
                 << std::right << std::setw(widths[2]) << static_cast<int>(row.iso_speed)
                 << std::right << std::setw(widths[3]) << std::fixed << std::setprecision(4) << row.dr_ev
                 << std::right << std::setw(widths[4]) << DataSourceToString(row.channel)
                 << std::right << std::setw(widths[5]) << row.samples_R
                 << std::right << std::setw(widths[6]) << row.samples_G1
                 << std::right << std::setw(widths[7]) << row.samples_G2
                 << std::right << std::setw(widths[8]) << row.samples_B << "\n";
    }
    return table_ss.str();
}

std::string FormatCsvHeader() {
    return "raw_file,SNRthreshold_db,ISO,DR_EV,raw_channel,samples_R,samples_G1,samples_G2,samples_B";
}

std::string FormatCsvRow(const FlatResultRow& row) {
    std::stringstream row_ss;
    row_ss << fs::path(row.filename).filename().string() << ","
           << std::fixed << std::setprecision(2) << row.snr_threshold_db << ","
           << static_cast<int>(row.iso_speed) << ","
           << std::fixed << std::setprecision(4) << row.dr_ev << ","
           << DataSourceToString(row.channel) << ","
           << row.samples_R << "," << row.samples_G1 << "," 
           << row.samples_G2 << "," << row.samples_B << "\n";
    return row_ss.str();
}
} // namespace Formatters