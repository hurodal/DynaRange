// File: src/core/engine/Reporting.hpp
/**
 * @file core/engine/Reporting.hpp
 * @brief Defines the functionality for generating final reports and plots.
 */
#pragma once
#include "Processing.hpp" // For ProcessingResult
#include <string>
#include <optional>
#include <map>

/**
 * @struct ReportOutput
 * @brief Contains the results of the reporting phase, primarily file paths.
 */
struct ReportOutput {
    /// @brief Path to the main summary plot image. Empty if not generated.
    std::optional<std::string> summary_plot_path; 
    
    /// @brief Maps each source RAW filename to the path of its generated individual plot.
    std::map<std::string, std::string> individual_plot_paths;

    /// @brief The final, absolute path where the CSV results file was saved.
    std::string final_csv_path;

    // Added the vector of results to this struct. This ensures the final,
    // sorted list of results is available to the GUI presenter.
    std::vector<DynamicRangeResult> dr_results;
    std::vector<CurveData> curve_data;
};

/**
 * @brief Generates all final output reports from the processing results.
 * @param results The aggregated results from the ProcessFiles function.
 * @param opts The program options.
 * @param log_stream The output stream for logging messages.
 * @return A ReportOutput struct containing paths to the generated plots.
 */
ReportOutput FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream);