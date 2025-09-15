// File: src/core/engine/Reporting.cpp
/**
 * @file core/engine/Reporting.cpp
 * @brief Implements the report generation logic.
 */
#include "Reporting.hpp"
#include "../graphics/Plotting.hpp"
#include "../utils/PathManager.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace { // Anonymous namespace for internal helper functions

/**
 * @brief Generates all individual SNR plot images.
 */
std::map<std::string, std::string> GenerateIndividualPlots(
    const std::vector<CurveData>& all_curves_data,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream)
{
    std::map<std::string, std::string> plot_paths_map;
    log_stream << "\nGenerating individual SNR plots..." << std::endl;
    for (const auto& curve : all_curves_data) {
        fs::path plot_path = paths.GetIndividualPlotPath(curve);
        plot_paths_map[curve.filename] = plot_path.string();

        std::stringstream title_ss;
        title_ss << fs::path(curve.filename).filename().string();
        
        if (!curve.camera_model.empty()) {
            title_ss << " (" << curve.camera_model;
            if (curve.iso_speed > 0) {
                title_ss << ", ISO " << static_cast<int>(curve.iso_speed);
            }
            title_ss << ")";
        }

        GenerateSnrPlot(plot_path.string(), title_ss.str(), curve.plot_label, curve.signal_ev, curve.snr_db, curve.poly_coeffs, opts, log_stream);
    }
    return plot_paths_map;
}

/**
 * @brief Generates the summary plot image.
 */
std::optional<std::string> GenerateSummaryPlotReport(
    const std::vector<CurveData>& all_curves_data,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream)
{
    if (all_curves_data.empty()) {
        return std::nullopt;
    }
    std::string camera_name = all_curves_data[0].camera_model;
    fs::path summary_plot_path = paths.GetSummaryPlotPath(camera_name);
    
    // The GenerateSummaryPlot from Plotting.cpp returns the path, so we return it upwards.
    return GenerateSummaryPlot(summary_plot_path.string(), camera_name, all_curves_data, opts, log_stream);
}

/**
 * @brief Generates the CSV file and the log table report.
 */
void GenerateCsvAndLogReport(
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream)
{
    log_stream << "\n--- Dynamic Range Results ---\n";
    
    std::stringstream header_log, header_csv;
    header_log << std::left << std::setw(30) << "RAW File";
    header_csv << "raw_file";
    
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
    
    std::ofstream csv_file(paths.GetCsvOutputPath());
    csv_file << header_csv.str() << "\n";

    for (const auto& res : all_results) {
        log_stream << std::left << std::setw(30) << fs::path(res.filename).filename().string();
        csv_file << fs::path(res.filename).filename().string();
        for (const double threshold : opts.snr_thresholds_db) {
            double value = res.dr_values_ev.count(threshold) ? res.dr_values_ev.at(threshold) : 0.0;
            log_stream << std::fixed << std::setprecision(4) << std::setw(20) << value;
            csv_file << "," << value;
        }
        log_stream << res.patches_used << std::endl;
        csv_file << "," << res.patches_used << "\n";
    }

    csv_file.close();
    log_stream << "\nResults saved to " << paths.GetCsvOutputPath().string() << std::endl;
}

} // end anonymous namespace


ReportOutput FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    PathManager paths(opts);
    ReportOutput output;

    output.individual_plot_paths = GenerateIndividualPlots(results.curve_data, opts, paths, log_stream);
    
    GenerateCsvAndLogReport(results.dr_results, opts, paths, log_stream);

    output.summary_plot_path = GenerateSummaryPlotReport(results.curve_data, opts, paths, log_stream);

    return output;
}