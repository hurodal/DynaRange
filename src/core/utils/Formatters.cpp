// File: src/core/utils/Formatters.cpp
/**
 * @file src/core/utils/Formatters.cpp
 * @brief Implements the data formatting utility functions.
 */
#include "Formatters.hpp"
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;
namespace Formatters {

std::string FormatResultHeader(const ProgramOptions& opts, FormatType type) {
    std::stringstream header_ss;
    const bool for_log = (type == FormatType::Log);

    if (for_log) {
        const int num_width = 18;
        const int patches_width = 10;
        header_ss << std::left << std::setw(20) << _("RAW File");
        
        for (const double threshold : opts.snr_thresholds_db) {
            std::stringstream col_name_ss;
            col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
            header_ss << std::right << std::setw(num_width) << col_name_ss.str();
        }
        header_ss << std::right << std::setw(patches_width) << _("Patches");

    } else { // CSV Format
        header_ss << "raw_file";
        for (const double threshold : opts.snr_thresholds_db) {
            std::stringstream col_name_ss;
            col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
            header_ss << "," << col_name_ss.str();
        }
        header_ss << ",patches_used";
    }

    return header_ss.str();
}

std::string FormatResultRow(const DynamicRangeResult& res, const ProgramOptions& opts, FormatType type) {
    std::stringstream row_ss;
    const bool for_log = (type == FormatType::Log);
    std::string filename = fs::path(res.filename).filename().string();

    if (for_log) {
        const int num_width = 18;
        const int patches_width = 10;
        row_ss << std::left << std::setw(20) << filename;
        
        for (const double threshold : opts.snr_thresholds_db) {
            double value = res.dr_values_ev.count(threshold) ?
                           res.dr_values_ev.at(threshold) : 0.0;
            row_ss << std::right << std::setw(num_width) << std::fixed << std::setprecision(4) << value;
        }
        row_ss << std::right << std::setw(patches_width) << res.patches_used;

    } else { // CSV Format
        row_ss << filename;
        for (const double threshold : opts.snr_thresholds_db) {
            double value = res.dr_values_ev.count(threshold) ?
                           res.dr_values_ev.at(threshold) : 0.0;
            row_ss << "," << std::fixed << std::setprecision(4) << value;
        }
        row_ss << "," << res.patches_used;
    }

    return row_ss.str();
}

} // namespace Formatters