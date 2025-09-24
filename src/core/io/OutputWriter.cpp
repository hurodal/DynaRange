// File: src/core/io/OutputWriter.cpp
/**
 * @file src/core/io/OutputWriter.cpp
 * @brief Implements the OutputWriter module.
 */
#include "OutputWriter.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <libintl.h>

#define _(string) gettext(string)

namespace { // Anonymous namespace for private helper functions

// This helper function was previously in Reporting.cpp
std::string GenerateCsvDataRow(const DynamicRangeResult& res, const ProgramOptions& opts) {
    std::stringstream row_ss;
    std::string filename = fs::path(res.filename).filename().string();
    row_ss << filename;

    for (const double threshold : opts.snr_thresholds_db) {
        double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
        row_ss << "," << std::fixed << std::setprecision(4) << value;
    }
    row_ss << "," << res.patches_used;
    return row_ss.str();
}

} // end anonymous namespace

namespace OutputWriter {

bool WritePng(cairo_surface_t* surface, const fs::path& path, std::ostream& log_stream) {
    if (!surface) return false;
    cairo_status_t status = cairo_surface_write_to_png(surface, path.c_str());
    if (status == CAIRO_STATUS_SUCCESS) {
        log_stream << _("  - Info: Plot saved to: ") << path.string() << std::endl;
        return true;
    }
    return false;
}

bool WriteCsv(
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    const fs::path& path,
    std::ostream& log_stream)
{
    std::ofstream csv_file(path);
    if (!csv_file.is_open()) {
        log_stream << "\n" << _("Error: Could not open CSV file for writing: ") << path.string() << std::endl;
        return false;
    }

    std::stringstream header_csv;
    header_csv << "raw_file";
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream col_name_ss;
        col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
        header_csv << "," << col_name_ss.str();
    }
    header_csv << ",patches_used";

    csv_file << header_csv.str() << std::endl;
    for (const auto& res : all_results) {
        csv_file << GenerateCsvDataRow(res, opts) << std::endl;
    }

    csv_file.close();
    log_stream << "\n" << _("Results saved to ") << path.string() << std::endl;
    return true;
}

} // namespace OutputWriter