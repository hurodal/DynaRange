// File: src/core/engine/Reporting.cpp
/**
 * @file core/engine/Reporting.cpp
 * @brief Implements the report generation logic using ArtifactFactory.
 */
#include "Reporting.hpp"
#include "../artifacts/data/ReportWriter.hpp"
#include "../artifacts/plot/PlotWriter.hpp"
#include "../utils/PathManager.hpp"
#include "../utils/OutputNamingContext.hpp"
#include "../utils/Formatters.hpp"
#include "../graphics/PlotBoundsCalculator.hpp"
#include "../graphics/PlotDataGenerator.hpp"
#include <filesystem>
#include <libintl.h>
#include <vector>
#include <map>    // For grouping results/curves by filename
#include <mutex>  // Potentially needed if parallelizing individual plots later

#define _(string) gettext(string)

namespace fs = std::filesystem;

/**
 * @brief Generates all final output reports from the processing results, using ArtifactFactory.
 * @details Uses reporting_params to determine naming context for artifacts. // Updated Doxygen
 * @param results The aggregated results from the ProcessFiles function.
 * @param reporting_params The consolidated parameters required for generating reports.
 * @param paths The PathManager for resolving output paths.
 * @param log_stream The output stream for logging messages.
 * @return A ReportOutput struct containing paths to the generated files and final results.
 */
ReportOutput FinalizeAndReport(
    const ProcessingResult& results,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream)
{
    ReportOutput output;
    if (results.dr_results.empty() && results.curve_data.empty()) {
        log_stream << _("Warning: No results available to report.") << std::endl;
        return output; // Return empty report
    }

    // --- Create the main OutputNamingContext ---
    OutputNamingContext ctx;
    if (!results.curve_data.empty()) {
        ctx.camera_name_exif = results.curve_data[0].camera_model;
    }

    // Determine effective_camera_name_for_output using the flags
    // passed via reporting_params (which mirror ProgramOptions).
    std::string effective_name = "";
    if (reporting_params.gui_use_camera_suffix) {
        if (reporting_params.gui_use_exif_camera_name) {
            effective_name = ctx.camera_name_exif; // Use EXIF name
        } else {
            effective_name = reporting_params.gui_manual_camera_name; // Use Manual name
        }
    }
    ctx.effective_camera_name_for_output = effective_name;

    ctx.raw_channels = reporting_params.raw_channels;
    ctx.plot_format = reporting_params.plot_format;
    
    // TODO: Populate ctx.user_csv_filename and ctx.user_print_patches_filename if needed.
    // This would require adding them to ReportingParameters as well.
    // For now, they default to empty, letting the Factory use standard names.

    // --- Generate CSV Report using Factory ---
    log_stream << "\n--- " << _("Dynamic Range Results") << " ---" << std::endl;
    auto sorted_rows = Formatters::FlattenAndSortResults(results.dr_results);
    log_stream << Formatters::FormatResultsTable(sorted_rows);

    std::optional<fs::path> csv_path_opt = ArtifactFactory::Report::CreateCsvReport(
        results.dr_results, ctx, paths, log_stream);
    if (csv_path_opt) {
        output.final_csv_path = csv_path_opt->string();
    } else {
        log_stream << _("Error: Failed to generate CSV report.") << std::endl;
    }

    // --- Generate Plots using Factory (if requested) ---
    if (reporting_params.generate_plot && !results.curve_data.empty()) {
        // Prepare ALL curve data points and calculate global bounds ONCE
        std::vector<CurveData> all_curves_with_points = results.curve_data;
        for (auto& curve : all_curves_with_points) {
            if (curve.curve_points.empty()) {
                curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
            }
        }
        const auto global_bounds = DynaRange::Graphics::CalculateGlobalBounds(all_curves_with_points);

        // --- Generate Summary Plot ---
        std::optional<fs::path> summary_plot_path_opt = ArtifactFactory::Plot::CreateSummaryPlot(
            all_curves_with_points, // Pass curves with points
            results.dr_results,
            ctx,
            reporting_params,
            paths,
            log_stream);
        if (summary_plot_path_opt) {
            output.summary_plot_path = summary_plot_path_opt->string();
        } else {
             log_stream << _("Error: Failed to generate summary plot.") << std::endl;
        }

        // --- Generate Individual Plots (using Factory) ---
        if (reporting_params.generate_individual_plots) {
            log_stream << "\n" << _("Generating individual SNR plots...") << std::endl;

            // Group curves and results by filename (necessary for iterating)
            std::map<std::string, std::vector<CurveData>> curves_by_file;
            for (const auto& curve : all_curves_with_points) { // Use curves with points
                curves_by_file[curve.filename].push_back(curve);
            }
            std::map<std::string, std::vector<DynamicRangeResult>> results_by_file;
            for (const auto& result : results.dr_results) {
                results_by_file[result.filename].push_back(result);
            }

            // Iterate and call Factory for each file
            // TODO: Consider parallelizing this loop if performance becomes an issue
            std::mutex log_mutex_individual; // Mutex if parallelized
            for (const auto& pair : curves_by_file) {
                const std::string& filename = pair.first;
                const std::vector<CurveData>& curves_for_this_file = pair.second;

                if (results_by_file.find(filename) == results_by_file.end()) {
                     continue; // Skip if no results for this file
                }
                const std::vector<DynamicRangeResult>& results_for_this_file = results_by_file.at(filename);

                // Create context specific to this individual plot
                OutputNamingContext individual_ctx = ctx; // Copy base context
                individual_ctx.iso_speed = curves_for_this_file[0].iso_speed; // Add ISO

                std::optional<fs::path> individual_plot_path_opt = ArtifactFactory::Plot::CreateIndividualPlot(
                    curves_for_this_file,
                    results_for_this_file,
                    individual_ctx,
                    reporting_params,
                    global_bounds, // Pass global bounds for consistent axes
                    paths,
                    log_stream // Pass main log stream (needs mutex if parallel)
                );

                if (individual_plot_path_opt) {
                    output.individual_plot_paths[filename] = individual_plot_path_opt->string();
                } else {
                     log_stream << _("Error: Failed to generate individual plot for: ") << filename << std::endl;
                }
            }
        }
    } // End if generate_plot

    // Populate final report structure with results data (for GUI presenter)
    output.dr_results = results.dr_results;
    output.curve_data = results.curve_data; // Store original curve data without generated points if needed elsewhere

    return output;
}