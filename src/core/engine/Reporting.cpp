// File: core/engine/Reporting.cpp
#include "Reporting.hpp"
#include "../graphics/Plotting.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// Generates the final reports: summary plot, individual plots, table, and CSV.
std::optional<std::string> FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    const auto& all_results = results.dr_results;
    const auto& all_curves_data = results.curve_data;
    fs::path output_dir_path = fs::path(opts.output_filename).parent_path();
    
    // --- ADDED: Responsibility to generate individual plots ---
    log_stream << "\nGenerating individual SNR plots..." << std::endl;
    for (const auto& curve : all_curves_data) {
        fs::path plot_path = output_dir_path / (fs::path(curve.name).stem().string() + "_snr_plot.png");
        GenerateSnrPlot(
            plot_path.string(), 
            fs::path(curve.name).filename().string(), 
            curve.signal_ev, 
            curve.snr_db, 
            curve.poly_coeffs, 
            opts, 
            log_stream
        );
    }
    
    // --- Generate summary plot ---
    std::optional<std::string> summary_plot_path_opt = std::nullopt;
    if (!all_curves_data.empty()) {
        std::string camera_name = all_curves_data[0].camera_model;
        summary_plot_path_opt = GenerateSummaryPlot(output_dir_path.string(), camera_name, all_curves_data, opts, log_stream);
    }

    // --- Generate console and CSV reports ---
    log_stream << "\n--- Dynamic Range Results ---\n";
    std::stringstream dr_header_ss;
    dr_header_ss << "DR(" << std::fixed << std::setprecision(2) << opts.snr_thresholds_db[0] << "dB)";
    log_stream << std::left << std::setw(30) << "RAW File" << std::setw(20) << dr_header_ss.str() << std::setw(15) << "DR(0dB)" << "Patches" << std::endl;
    log_stream << std::string(80, '-') << std::endl;
    for (const auto& res : all_results) {
        log_stream << std::left << std::setw(30) << fs::path(res.filename).filename().string() 
                   << std::fixed << std::setprecision(4) << std::setw(20) << res.dr_12db 
                   << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_0db 
                   << res.patches_used << std::endl;
    }
    
    std::ofstream csv_file(opts.output_filename);
    csv_file << "raw_file,DR_" << opts.snr_thresholds_db[0] << "dB,DR_0dB,patches_used\n";
    for (const auto& res : all_results) {
        csv_file << fs::path(res.filename).filename().string() << "," << res.dr_12db << "," << res.dr_0db << "," << res.patches_used << "\n";
    }
    csv_file.close();
    log_stream << "\nResults saved to " << opts.output_filename << std::endl;
    
    return summary_plot_path_opt;
}