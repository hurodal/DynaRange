// File: src/core/engine/Reporting.cpp
/**
 * @file core/engine/Reporting.cpp
 * @brief Implements the report generation logic.
 */
#include "Reporting.hpp"
#include "../graphics/Plotting.hpp"
#include "../utils/PathManager.hpp"
#include "../io/OutputWriter.hpp"
#include "../utils/Formatters.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace { // Anonymous namespace for internal helper functions

/**
 * @brief Generates the summary plot image.
 */
std::optional<std::string> GenerateSummaryPlotReport(
    const std::vector<CurveData>& all_curves_data,
    const std::vector<DynamicRangeResult>& all_dr_results,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream)
{
    if (all_curves_data.empty()) {
        return std::nullopt;
    }
    std::string camera_name = all_curves_data[0].camera_model;
    fs::path summary_plot_path = paths.GetSummaryPlotPath(camera_name);
    
    return GenerateSummaryPlot(summary_plot_path.string(), camera_name, all_curves_data, all_dr_results, opts, log_stream);
}

} // end anonymous namespace

ReportOutput FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    PathManager paths(opts);
    ReportOutput output;

    // Save the debug patch image if it was requested and generated
    if (!opts.print_patch_filename.empty() && results.debug_patch_image.has_value()) {
        fs::path debug_path = paths.GetCsvOutputPath().parent_path() / opts.print_patch_filename;
        OutputWriter::WriteDebugImage(*results.debug_patch_image, debug_path, log_stream);
    }

    output.final_csv_path = paths.GetCsvOutputPath().string();
    // This call is correct, as GenerateIndividualPlots is now declared in Plotting.hpp
    output.individual_plot_paths = GenerateIndividualPlots(results.curve_data, results.dr_results, opts, paths, log_stream);
    
    // Generate the log report by calling the Formatters module directly.
    log_stream << "\n--- " << _("Dynamic Range Results") << " ---" << std::endl;
    log_stream << Formatters::FormatResultsTable(results.dr_results, opts);
    
    // Generate the CSV file by calling the OutputWriter module.
    OutputWriter::WriteCsv(results.dr_results, opts, paths.GetCsvOutputPath(), log_stream);
    
    output.summary_plot_path = GenerateSummaryPlotReport(results.curve_data, results.dr_results, opts, paths, log_stream);

    output.dr_results = results.dr_results;
    return output;
}