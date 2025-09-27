// File: src/core/io/OutputWriter.cpp
/**
 * @file src/core/io/OutputWriter.cpp
 * @brief Implements the OutputWriter module.
 */
#include "OutputWriter.hpp"
#include "../utils/Formatters.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <libintl.h>

#define _(string) gettext(string)

namespace OutputWriter {

// This function existed previously and has been modified.
bool WritePng(cairo_surface_t* surface, const fs::path& path, std::ostream& log_stream) {
    if (!surface) return false;
    // MODIFIED: Use path.string().c_str() for cross-platform compatibility.
    // This converts the path to a std::string (using char) before getting the C-style string,
    // which resolves the wchar_t* vs char* conflict on Windows.
    cairo_status_t status = cairo_surface_write_to_png(surface, path.string().c_str());
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

    //Formatting logic is now delegated to the Formatters module.
    csv_file << Formatters::FormatResultHeader(opts, Formatters::FormatType::Csv) << std::endl;
    for (const auto& res : all_results) {
        csv_file << Formatters::FormatResultRow(res, opts, Formatters::FormatType::Csv) << std::endl;
    }

    csv_file.close();
    log_stream << "\n" << _("Results saved to ") << path.string() << std::endl;
    return true;
}

} // namespace OutputWriter