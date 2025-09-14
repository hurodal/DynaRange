/**
 * @file core/Engine.hpp
 * @brief Declares the main orchestrator function for the dynamic range analysis.
 */
#pragma once
#include "Arguments.hpp"
#include "engine/Reporting.hpp" // Needed for ReportOutput struct
#include <ostream>

/**
 * @brief Runs the complete dynamic range analysis workflow.
 * @param opts Program options, passed by reference.
 * @param log_stream The stream for logging all output messages.
 * @return A ReportOutput struct containing the paths to any generated plot files.
 * The 'summary_plot_path' will be std::nullopt on failure.
 */
ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream);