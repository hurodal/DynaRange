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

// All key default values for the test chart are now defined here.
// This is now the single source of truth for the chart's default state.
constexpr int DEFAULT_CHART_R = 162;
constexpr int DEFAULT_CHART_G = 64;
constexpr int DEFAULT_CHART_B = 104;
constexpr double DEFAULT_CHART_INV_GAMMA = 1.4;
constexpr int DEFAULT_CHART_DIM_X = 1920;
constexpr int DEFAULT_CHART_ASPECT_W = 3;
constexpr int DEFAULT_CHART_ASPECT_H = 2;
constexpr int DEFAULT_CHART_PATCHES_M = 4; // Rows
constexpr int DEFAULT_CHART_PATCHES_N = 6; // Cols

/**
 * @struct ChartGeneratorOptions
 * @brief Holds the final, validated parameters for chart generation.
 */
struct ChartGeneratorOptions {
    // All members are initialized from the central constants.
    int R = DEFAULT_CHART_R;
    int G = DEFAULT_CHART_G;
    int B = DEFAULT_CHART_B;
    double invgamma = DEFAULT_CHART_INV_GAMMA;
    int dim_x = DEFAULT_CHART_DIM_X;
    int aspect_w = DEFAULT_CHART_ASPECT_W;
    int aspect_h = DEFAULT_CHART_ASPECT_H;
    int patches_m = DEFAULT_CHART_PATCHES_M;
    int patches_n = DEFAULT_CHART_PATCHES_N;
};

/**
 * @brief Parses the chart-related parameters from ProgramOptions.
 * @param opts The ProgramOptions struct populated by the argument parser.
 * @param log_stream A stream for logging error messages.
 * @return An std::optional containing the validated chart options on success,
 * or std::nullopt on failure.
 */
std::optional<ChartGeneratorOptions> ParseChartOptions(const ProgramOptions& opts, std::ostream& log_stream);