// File: src/core/utils/Formatters.cpp
/**
 * @file src/core/utils/Formatters.cpp
 * @brief Implements the data formatting utility functions.
 */
#include "Formatters.hpp"
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <libintl.h>

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

std::string FormatResultsTable(const std::vector<DynamicRangeResult>& all_results, const ProgramOptions& opts) {
    if (all_results.empty()) {
        return "";
    }

    // --- Define Headers and initial widths for the new "long" format ---
    const std::vector<std::string> headers = {
        _("raw_file"), _("SNRthreshold_db"), _("raw_channel"), 
        "samples_R", "samples_G1", "samples_G2", "samples_B", "DR_EV"
    };
    std::vector<size_t> widths;
    for(const auto& h : headers) {
        widths.push_back(h.length());
    }

    // --- Measure max widths from data ---
    for (const auto& res : all_results) {
        widths[0] = std::max(widths[0], fs::path(res.filename).filename().string().length());
        widths[3] = std::max(widths[3], std::to_string(res.samples_R).length());
        widths[4] = std::max(widths[4], std::to_string(res.samples_G1).length());
        widths[5] = std::max(widths[5], std::to_string(res.samples_G2).length());
        widths[6] = std::max(widths[6], std::to_string(res.samples_B).length());

        for (const auto& pair : res.dr_values_ev) {
            std::stringstream ss_thresh, ss_dr;
            ss_thresh << std::fixed << std::setprecision(2) << pair.first;
            ss_dr << std::fixed << std::setprecision(4) << pair.second;
            widths[1] = std::max(widths[1], ss_thresh.str().length());
            widths[7] = std::max(widths[7], ss_dr.str().length());
        }
    }

    // --- Build Table ---
    auto add_padding = [](size_t& width) { width += 2; };
    for(auto& w : widths) add_padding(w);

    std::stringstream table_ss;
    table_ss << std::left << std::setw(widths[0]) << headers[0]
             << std::right << std::setw(widths[1]) << headers[1]
             << std::left << std::setw(widths[2]) << headers[2]
             << std::right << std::setw(widths[3]) << headers[3]
             << std::right << std::setw(widths[4]) << headers[4]
             << std::right << std::setw(widths[5]) << headers[5]
             << std::right << std::setw(widths[6]) << headers[6]
             << std::right << std::setw(widths[7]) << headers[7] << "\n";

    size_t total_width = 0;
    for(auto w : widths) total_width += w;
    table_ss << std::string(total_width, '-') << "\n";

    // Data rows
    for (const auto& res : all_results) {
        for (const auto& pair : res.dr_values_ev) {
            table_ss << std::left << std::setw(widths[0]) << fs::path(res.filename).filename().string()
                     << std::right << std::setw(widths[1]) << std::fixed << std::setprecision(2) << pair.first
                     << std::left << std::setw(widths[2]) << DataSourceToString(res.channel)
                     << std::right << std::setw(widths[3]) << res.samples_R
                     << std::right << std::setw(widths[4]) << res.samples_G1
                     << std::right << std::setw(widths[5]) << res.samples_G2
                     << std::right << std::setw(widths[6]) << res.samples_B
                     << std::right << std::setw(widths[7]) << std::fixed << std::setprecision(4) << pair.second << "\n";
        }
    }
    return table_ss.str();
}

std::string FormatCsvHeader() {
    return "raw_file,SNRthreshold_db,raw_channel,samples_R,samples_G1,samples_G2,samples_B,DR_EV";
}

std::string FormatCsvRows(const DynamicRangeResult& res) {
    std::stringstream rows_ss;
    for (const auto& pair : res.dr_values_ev) {
        rows_ss << fs::path(res.filename).filename().string() << ","
                << std::fixed << std::setprecision(8) << pair.first << ","
                << DataSourceToString(res.channel) << ","
                << res.samples_R << "," << res.samples_G1 << "," 
                << res.samples_G2 << "," << res.samples_B << ","
                << std::fixed << std::setprecision(8) << pair.second << "\n";
    }
    return rows_ss.str();
}

} // namespace Formatters