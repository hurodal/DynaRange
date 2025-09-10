// File: core/engine/Reporting.cpp
#include "Reporting.hpp"
#include "../graphics/Plotting.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// Generates the final reports: summary plot, individual plots, table, and CSV.
// This function now dynamically builds the results table based on the calculated DR values.
std::optional<std::string> FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    const auto& all_results = results.dr_results;
    const auto& all_curves_data = results.curve_data;
    fs::path output_dir_path = fs::path(opts.output_filename).parent_path();
    
    log_stream << "\nGenerating individual SNR plots..." << std::endl;
    for (const auto& curve : all_curves_data) {
        fs::path plot_path = output_dir_path / (fs::path(curve.name).stem().string() + "_snr_plot.png");
        GenerateSnrPlot(plot_path.string(), fs::path(curve.name).filename().string(), curve.signal_ev, curve.snr_db, curve.poly_coeffs, opts, log_stream);
    }
    
    std::optional<std::string> summary_plot_path_opt = std::nullopt;
    if (!all_curves_data.empty()) {
        std::string camera_name = all_curves_data[0].camera_model;
        summary_plot_path_opt = GenerateSummaryPlot(output_dir_path.string(), camera_name, all_curves_data, opts, log_stream);
    }

    // --- Dynamic report generation ---
    log_stream << "\n--- Dynamic Range Results ---\n";
    
    // 1. Build headers dynamically from the requested thresholds
    std::stringstream header_log, header_csv;
    header_log << std::left << std::setw(30) << "RAW File";
    header_csv << "raw_file";
    // The columns will be ordered as they appear in the options vector
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream col_name_ss;
        col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
        header_log << std::setw(20) << col_name_ss.str();
        header_csv << "," << col_name_ss.str();
    }
    header_log << "Patches";
    header_csv << ",patches_used";

    log_stream << header_log.str() << std::endl;
    log_stream << std::string(header_log.str().length(), '-') << std::endl;
    
    std::ofstream csv_file(opts.output_filename);
    csv_file << header_csv.str() << "\n";

    // 2. Populate rows dynamically from the results map
    for (const auto& res : all_results) {
        log_stream << std::left << std::setw(30) << fs::path(res.filename).filename().string();
        csv_file << fs::path(res.filename).filename().string();
        // Iterate in the same order to match the headers
        for (const double threshold : opts.snr_thresholds_db) {
            // Find the value in the map; default to 0.0 if not found
            double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
            log_stream << std::fixed << std::setprecision(4) << std::setw(20) << value;
            csv_file << "," << value;
        }
        log_stream << res.patches_used << std::endl;
        csv_file << "," << res.patches_used << "\n";
    }

    csv_file.close();
    log_stream << "\nResults saved to " << opts.output_filename << std::endl;
    
    return summary_plot_path_opt;
}