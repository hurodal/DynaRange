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

std::string FormatResultsTable(const std::vector<DynamicRangeResult>& all_results, const ProgramOptions& opts) {
    if (all_results.empty()) {
        return "";
    }

    // --- Define Headers and initial widths ---
    const std::string file_header = _("raw_file");
    const std::string channel_header = _("raw_channel");
    std::vector<std::string> dr_headers;
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream ss;
        ss << "DR_EV_" << std::fixed << std::setprecision(1) << threshold << "dB";
        dr_headers.push_back(ss.str());
    }
    const std::string r_header = "samples_R", g1_header = "samples_G1", g2_header = "samples_G2", b_header = "samples_B";
    
    size_t file_width = file_header.length(), channel_width = channel_header.length();
    std::vector<size_t> dr_widths;
    for(const auto& header : dr_headers) dr_widths.push_back(header.length());
    size_t r_width = r_header.length(), g1_width = g1_header.length(), g2_width = g2_header.length(), b_width = b_header.length();

    // --- Measure max widths from data ---
    for (const auto& res : all_results) {
        file_width = std::max(file_width, fs::path(res.filename).filename().string().length());
        for (size_t i = 0; i < opts.snr_thresholds_db.size(); ++i) {
            double threshold = opts.snr_thresholds_db[i];
            double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
            std::stringstream ss;
            ss << std::fixed << std::setprecision(4) << value;
            dr_widths[i] = std::max(dr_widths[i], ss.str().length());
        }
        r_width = std::max(r_width, std::to_string(res.samples_R).length());
        g1_width = std::max(g1_width, std::to_string(res.samples_G1).length());
        g2_width = std::max(g2_width, std::to_string(res.samples_G2).length());
        b_width = std::max(b_width, std::to_string(res.samples_B).length());
    }

    // --- Build Table ---
    auto add_padding = [](size_t& width) { width += 2; };
    add_padding(file_width); add_padding(channel_width);
    for(auto& w : dr_widths) add_padding(w);
    add_padding(r_width); add_padding(g1_width); add_padding(g2_width); add_padding(b_width);

    std::stringstream table_ss;
    table_ss << std::left << std::setw(file_width) << file_header << std::left << std::setw(channel_width) << channel_header;
    for (size_t i = 0; i < dr_headers.size(); ++i) table_ss << std::right << std::setw(dr_widths[i]) << dr_headers[i];
    table_ss << std::right << std::setw(r_width) << r_header << std::right << std::setw(g1_width) << g1_header
             << std::right << std::setw(g2_width) << g2_header << std::right << std::setw(b_width) << b_header << "\n";
    
    // Separator line
    size_t total_width = file_width + channel_width + r_width + g1_width + g2_width + b_width;
    for(auto w : dr_widths) total_width += w;
    table_ss << std::string(total_width, '-') << "\n";

    // Data rows
    for (const auto& res : all_results) {
        table_ss << std::left << std::setw(file_width) << fs::path(res.filename).filename().string();
        table_ss << std::left << std::setw(channel_width) << DataSourceToString(res.channel);
        for (size_t i = 0; i < opts.snr_thresholds_db.size(); ++i) {
            double threshold = opts.snr_thresholds_db[i];
            double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
            table_ss << std::right << std::setw(dr_widths[i]) << std::fixed << std::setprecision(4) << value;
        }
        table_ss << std::right << std::setw(r_width) << res.samples_R << std::right << std::setw(g1_width) << res.samples_G1
                 << std::right << std::setw(g2_width) << res.samples_G2 << std::right << std::setw(b_width) << res.samples_B << "\n";
    }
    return table_ss.str();
}

std::string FormatCsvHeader(const ProgramOptions& opts) {
    std::stringstream header_ss;
    header_ss << "raw_file,raw_channel";
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream col_name_ss;
        col_name_ss << "DR_EV_" << std::fixed << std::setprecision(1) << threshold << "dB";
        header_ss << "," << col_name_ss.str();
    }
    header_ss << ",samples_R,samples_G1,samples_G2,samples_B";
    return header_ss.str();
}

std::string FormatCsvRow(const DynamicRangeResult& res, const ProgramOptions& opts) {
    std::stringstream row_ss;
    row_ss << fs::path(res.filename).filename().string();
    row_ss << "," << DataSourceToString(res.channel);
    for (const double threshold : opts.snr_thresholds_db) {
        double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
        row_ss << "," << std::fixed << std::setprecision(8) << value;
    }
    row_ss << "," << res.samples_R << "," << res.samples_G1 << "," << res.samples_G2 << "," << res.samples_B;
    return row_ss.str();
}

} // namespace Formatters