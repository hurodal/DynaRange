/**
 * @file core/engine/Reporting.hpp
 * @brief Defines the functionality for generating final reports and plots.
 */
#pragma once
#include "Processing.hpp"
#include <string>
#include <optional>

/**
 * @brief Generates all final output reports from the processing results.
 * @details This includes a summary plot of all curves, individual SNR plots,
 * a results table printed to the log, and a CSV output file.
 * @param results The aggregated results from the ProcessFiles function.
 * @param opts The program options.
 * @param log_stream The output stream for logging messages.
 * @return An optional string containing the path to the generated summary plot,
 * or std::nullopt if no plot was created.
 */
std::optional<std::string> FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream);