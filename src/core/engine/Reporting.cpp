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
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream)
{
    if (all_curves_data.empty()) {
        return std::nullopt;
    }
    std::string camera_name = all_curves_data[0].camera_model;
    fs::path summary_plot_path = paths.GetSummaryPlotPath(camera_name, reporting_params.raw_channels, reporting_params.plot_format);
    
    return GenerateSummaryPlot(summary_plot_path.string(), camera_name, all_curves_data, all_dr_results, reporting_params, log_stream);
}

}// end anonymous namespace

ReportOutput FinalizeAndReport(
    const ProcessingResult& results,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream)
{
    ReportOutput output;

    // Flatten and sort the results before any output is generated.
    auto sorted_rows = Formatters::FlattenAndSortResults(results.dr_results);

    output.final_csv_path = paths.GetCsvOutputPath().string();
    output.individual_plot_paths = GenerateIndividualPlots(results.curve_data, results.dr_results, reporting_params, paths, log_stream);
    
    log_stream << "\n--- " << _("Dynamic Range Results") << " ---" << std::endl;
    log_stream << Formatters::FormatResultsTable(sorted_rows);
    
    OutputWriter::WriteCsv(sorted_rows, paths.GetCsvOutputPath(), log_stream);
    
    output.summary_plot_path = GenerateSummaryPlotReport(results.curve_data, results.dr_results, reporting_params, paths, log_stream);

    output.dr_results = results.dr_results;
    return output;
}