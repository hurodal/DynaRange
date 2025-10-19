// File: src/core/arguments/ChartOptionsParser.hpp
/**
 * @file ChartOptionsParser.hpp
 * @brief Declares a parser for chart generation options.
 * @details Adheres to SRP by encapsulating the logic for parsing and validating
 * chart-specific parameters from the command line. Includes chart defaults.
 */
#pragma once
#include "ArgumentsOptions.hpp" // Includes definition of DEFAULT_CHART_PATCHES_M/N
#include <optional>
#include <ostream>

// Default values for chart colour, gamma, dimensions, and aspect ratio remain here
// as they are primarily used by the ChartGeneratorOptions struct defined below.
constexpr int DEFAULT_CHART_R = 255;
constexpr int DEFAULT_CHART_G = 101;
constexpr int DEFAULT_CHART_B = 164;
constexpr double DEFAULT_CHART_INV_GAMMA = 1.4;
constexpr int DEFAULT_CHART_DIM_X = 1920;
constexpr int DEFAULT_CHART_ASPECT_W = 3;
constexpr int DEFAULT_CHART_ASPECT_H = 2;

/**
 * @struct ChartGeneratorOptions
 * @brief Holds the final, validated parameters for chart generation.
 * @details Initializes members using the default constants defined above
 * and those included from ArgumentsOptions.hpp.
 */
struct ChartGeneratorOptions {
    // Color and Gamma
    int R = DEFAULT_CHART_R;
    int G = DEFAULT_CHART_G;
    int B = DEFAULT_CHART_B;
    double invgamma = DEFAULT_CHART_INV_GAMMA;
    // Dimensions and Aspect Ratio
    int dim_x = DEFAULT_CHART_DIM_X;
    int aspect_w = DEFAULT_CHART_ASPECT_W;
    int aspect_h = DEFAULT_CHART_ASPECT_H;
    // Patches Grid - Uses defaults from ArgumentsOptions.hpp
    int patches_m = DEFAULT_CHART_PATCHES_M;
    int patches_n = DEFAULT_CHART_PATCHES_N;
};

/**
 * @brief Parses the chart-related parameters from ProgramOptions into ChartGeneratorOptions.
 * @param opts The ProgramOptions struct populated by the argument parser, containing raw chart parameters.
 * @param log_stream A stream for logging potential error messages during parsing.
 * @return An std::optional containing the validated ChartGeneratorOptions on success,
 * or std::nullopt if parsing fails (e.g., invalid parameter format).
 */
std::optional<ChartGeneratorOptions> ParseChartOptions(const ProgramOptions& opts, std::ostream& log_stream);