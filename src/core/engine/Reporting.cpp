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
#include <libintl.h>

#define _(string) gettext(string)

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
    log_stream << "\n" << _("Generating individual SNR plots...") << std::endl;
    for (const auto& curve : all_curves_data) {
        fs::path plot_path = paths.GetIndividualPlotPath(curve);
        plot_paths_map[curve.filename] = plot_path.string();

        std::stringstream title_ss;
        title_ss << fs::path(curve.filename).filename().string();
        
        if (!curve.camera_model.empty()) {
            title_ss << " (" << curve.camera_model;
            if (curve.iso_speed > 0) {
                title_ss << ", " << _("ISO ") << static_cast<int>(curve.iso_speed);
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
 * @brief Generates a single data row as a formatted string for either log or CSV.
 * @param res The DynamicRangeResult for the row.
 * @param opts The program options (for SNR thresholds).
 * @param for_log If true, formats for console log (with setw). If false, formats for CSV (comma-separated).
 * @return A string containing the formatted row.
 */
std::string GenerateDataRow(const DynamicRangeResult& res, const ProgramOptions& opts, bool for_log) {
    std::stringstream row_ss;
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
            row_ss << "," << value;
        }
    }

    if (for_log) {
        row_ss << res.patches_used;
    } else {
        row_ss << "," << res.patches_used;
    }

    return row_ss.str();
}

/**
 * @brief (SRP) Generates a formatted table of results for the console log.
 */
void GenerateLogReport(
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    log_stream << "\n--- " << _("Dynamic Range Results") << " ---" << std::endl;
    // --- Generate Header for Log ---
    std::stringstream header_log;
    header_log << std::left << std::setw(30) << _("RAW File");
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream col_name_ss;
        col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << _("dB)");
        header_log << std::setw(20) << col_name_ss.str();
    }
    header_log << _("Patches");

    // --- Print Header and Rows to Log ---
    log_stream << header_log.str() << std::endl;
    log_stream << std::string(header_log.str().length(), '-') << std::endl;
    
    for (const auto& res : all_results) {
        log_stream << GenerateDataRow(res, opts, true) << std::endl;
    }
}

/**
 * @brief (SRP) Generates a CSV file with the analysis results.
 */
void GenerateCsvReport(
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // --- Open CSV file ---
    std::ofstream csv_file(paths.GetCsvOutputPath());
    if (!csv_file.is_open()) {
        log_stream << "\n" << _("Error: Could not open CSV file for writing: ") << paths.GetCsvOutputPath() << std::endl;
        return;
    }

    // --- Generate Header for CSV ---
    std::stringstream header_csv;
    header_csv << "raw_file";
    for (const double threshold : opts.snr_thresholds_db) {
        std::stringstream col_name_ss;
        col_name_ss << "DR(" << std::fixed << std::setprecision(1) << threshold << "dB)";
        header_csv << "," << col_name_ss.str();
    }
    header_csv << ",patches_used";

    // --- Write Header and Rows to CSV ---
    csv_file << header_csv.str() << std::endl;
    for (const auto& res : all_results) {
        csv_file << GenerateDataRow(res, opts, false) << std::endl;
    }

    csv_file.close();
    log_stream << "\n" << _("Results saved to ") << paths.GetCsvOutputPath().string() << std::endl;
}

} // end anonymous namespace

ReportOutput FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    PathManager paths(opts);
    ReportOutput output;

    // Store the definitive CSV path in the output report.
    output.final_csv_path = paths.GetCsvOutputPath().string();
    
    output.individual_plot_paths = GenerateIndividualPlots(results.curve_data, opts, paths, log_stream);
    
    // Call the two new, single-responsibility functions
    GenerateLogReport(results.dr_results, opts, log_stream);
    GenerateCsvReport(results.dr_results, opts, paths, log_stream);

    output.summary_plot_path = GenerateSummaryPlotReport(results.curve_data, opts, paths, log_stream);

    return output;
}