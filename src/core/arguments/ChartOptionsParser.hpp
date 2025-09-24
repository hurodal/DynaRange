// File: src/core/arguments/ChartOptionsParser.hpp
/**
 * @file ChartOptionsParser.hpp
 * @brief Declares a parser for chart generation options.
 * @details Adheres to SRP by encapsulating the logic for parsing and validating
 * chart-specific parameters from the command line.
 */
#pragma once
#include "ProgramOptions.hpp"
#include <optional>
#include <ostream>

/**
 * @struct ChartGeneratorOptions
 * @brief Holds the final, validated parameters for chart generation.
 */
struct ChartGeneratorOptions {
    int R, G, B;
    double invgamma;
    int dim_x, aspect_w, aspect_h;
    int patches_m, patches_n;
};

/**
 * @brief Parses the chart-related parameters from ProgramOptions.
 * @param opts The ProgramOptions struct populated by the argument parser.
 * @param log_stream A stream for logging error messages.
 * @return An std::optional containing the validated chart options on success,
 * or std::nullopt on failure.
 */
std::optional<ChartGeneratorOptions> ParseChartOptions(const ProgramOptions& opts, std::ostream& log_stream);