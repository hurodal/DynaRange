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
        header_ss << std::left << std::setw(30) << _("RAW File");
    } else {
        header_ss << "raw_file";
    }

    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream col_name_ss;
        col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
        if (for_log) {
            header_ss << std::setw(20) << col_name_ss.str();
        } else {
            header_ss << "," << col_name_ss.str();
        }
    }

    if (for_log) {
        header_ss << _("Patches");
    } else {
        header_ss << ",patches_used";
    }

    return header_ss.str();
}

std::string FormatResultRow(const DynamicRangeResult& res, const ProgramOptions& opts, FormatType type) {
    std::stringstream row_ss;
    const bool for_log = (type == FormatType::Log);
    std::string filename = fs::path(res.filename).filename().string();

    if (for_log) {
        row_ss << std::left << std::setw(30) << filename;
    } else {
        row_ss << filename;
    }

    for (const double threshold : opts.snr_thresholds_db) {
        double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
        if (for_log) {
            row_ss << std::fixed << std::setprecision(4) << std::setw(20) << value;
        } else {
            row_ss << "," << std::fixed << std::setprecision(4) << value;
        }
    }

    if (for_log) {
        row_ss << res.patches_used;
    } else {
        row_ss << "," << res.patches_used;
    }

    return row_ss.str();
}

} // namespace Formatters