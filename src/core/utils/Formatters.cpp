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

std::string FormatResultsTable(const std::vector<DynamicRangeResult>& all_results, const ProgramOptions& opts) {
    if (all_results.empty()) {
        return "";
    }

    // --- 1. Fase de Medición ---
    
    // Nombres de las cabeceras
    const std::string file_header = _("RAW File");
    std::vector<std::string> dr_headers;
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream ss;
        ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
        dr_headers.push_back(ss.str());
    }
    const std::string patches_header = _("Patches");

    // Anchos de columna (inicializados con el ancho de la cabecera)
    size_t file_width = file_header.length();
    std::vector<size_t> dr_widths;
    for(const auto& header : dr_headers) {
        dr_widths.push_back(header.length());
    }
    size_t patches_width = patches_header.length();

    // Recorrer los datos para encontrar los anchos máximos
    for (const auto& res : all_results) {
        file_width = std::max(file_width, fs::path(res.filename).filename().string().length());
        
        for (size_t i = 0; i < opts.snr_thresholds_db.size(); ++i) {
            double threshold = opts.snr_thresholds_db[i];
            double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
            std::stringstream ss;
            ss << std::fixed << std::setprecision(4) << value;
            dr_widths[i] = std::max(dr_widths[i], ss.str().length());
        }

        patches_width = std::max(patches_width, std::to_string(res.patches_used).length());
    }

    // Añadir padding
    file_width += 2;
    for(auto& w : dr_widths) { w += 2; }
    patches_width += 2;
    
    // --- 2. Fase de Construcción ---
    
    std::stringstream table_ss;
    size_t total_width = file_width;

    // Construir cabecera
    table_ss << std::left << std::setw(file_width) << file_header;
    for (size_t i = 0; i < dr_headers.size(); ++i) {
        table_ss << std::right << std::setw(dr_widths[i]) << dr_headers[i];
        total_width += dr_widths[i];
    }
    table_ss << std::right << std::setw(patches_width) << patches_header;
    total_width += patches_width;
    table_ss << "\n" << std::string(total_width, '-') << "\n";

    // Construir filas de datos
    for (const auto& res : all_results) {
        table_ss << std::left << std::setw(file_width) << fs::path(res.filename).filename().string();
        for (size_t i = 0; i < opts.snr_thresholds_db.size(); ++i) {
            double threshold = opts.snr_thresholds_db[i];
            double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
            table_ss << std::right << std::setw(dr_widths[i]) << std::fixed << std::setprecision(4) << value;
        }
        table_ss << std::right << std::setw(patches_width) << res.patches_used << "\n";
    }

    return table_ss.str();
}

std::string FormatCsvHeader(const ProgramOptions& opts) {
    std::stringstream header_ss;
    header_ss << "raw_file";
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream col_name_ss;
        col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
        header_ss << "," << col_name_ss.str();
    }
    header_ss << ",patches_used";
    return header_ss.str();
}

std::string FormatCsvRow(const DynamicRangeResult& res, const ProgramOptions& opts) {
    std::stringstream row_ss;
    row_ss << fs::path(res.filename).filename().string();
    for (const double threshold : opts.snr_thresholds_db) {
        double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
        row_ss << "," << std::fixed << std::setprecision(4) << value;
    }
    row_ss << "," << res.patches_used;
    return row_ss.str();
}

} // namespace Formatters