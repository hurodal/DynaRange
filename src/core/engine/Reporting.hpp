// File: src/core/engine/Reporting.hpp
/**
 * @file core/engine/Reporting.hpp
 * @brief Defines the functionality for generating final reports and plots.
 */
#pragma once
#include "processing/Processing.hpp" // For ProcessingResult
#include "../arguments/ArgumentsOptions.hpp" // For PlottingDetails, etc.
#include "../graphics/Constants.hpp" // For PlotOutputFormat
#include <string>
#include <optional>
#include <vector>

// Forward declaration
class PathManager;

/**
 * @struct ReportingParameters
 * @brief (New struct) Encapsulates all parameters needed for the reporting phase.
 * @details This decouples the reporting engine from the main ProgramOptions struct.
 */
struct ReportingParameters {
    // General reporting settings
    RawChannelSelection raw_channels;
    // Plot-specific settings
    bool generate_plot;
    DynaRange::Graphics::Constants::PlotOutputFormat plot_format;
    PlottingDetails plot_details;
    int plot_command_mode;
    std::string generated_command;
    // Data for plot info box
    double dark_value;
    double saturation_value;
    bool black_level_is_default;
    bool saturation_level_is_default;
    std::vector<double> snr_thresholds_db;
};

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
 * @param reporting_params The consolidated parameters required for generating reports.
 * @param paths The PathManager for resolving output paths.
 * @param log_stream The output stream for logging messages.
 * @return A ReportOutput struct containing paths to the generated plots.
 */
ReportOutput FinalizeAndReport(
    const ProcessingResult& results,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream);