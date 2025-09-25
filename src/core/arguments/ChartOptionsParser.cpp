// File: src/core/arguments/ChartOptionsParser.cpp
/**
 * @file ChartOptionsParser.cpp
 * @brief Implements the chart options parser.
 */
#include "ChartOptionsParser.hpp"
#include <iostream>
#include <libintl.h>

#define _(string) gettext(string)

// This function already existed and is now updated.
std::optional<ChartGeneratorOptions> ParseChartOptions(const ProgramOptions& opts, std::ostream& log_stream) {
    ChartGeneratorOptions chart_opts;
    // Set default values from the user manual
    chart_opts.R = 255;
    chart_opts.G = 101;
    chart_opts.B = 164;
    chart_opts.invgamma = 1.4;
    chart_opts.dim_x = 1920;
    chart_opts.aspect_w = 4;
    chart_opts.aspect_h = 3;
    chart_opts.patches_m = 7;
    chart_opts.patches_n = 11;

    try {
        // Parse colour params if provided
        if (!opts.chart_colour_params.empty() && !opts.chart_colour_params[0].empty()) {
            if (opts.chart_colour_params.size() >= 1) chart_opts.R = std::stoi(opts.chart_colour_params[0]);
            if (opts.chart_colour_params.size() >= 2) chart_opts.G = std::stoi(opts.chart_colour_params[1]);
            if (opts.chart_colour_params.size() >= 3) chart_opts.B = std::stoi(opts.chart_colour_params[2]);
            if (opts.chart_colour_params.size() >= 4) chart_opts.invgamma = std::stod(opts.chart_colour_params[3]);
        }
        // Parse size params if provided, overwriting the default
        if (!opts.chart_params.empty()) {
            chart_opts.dim_x = opts.chart_params[0];
            chart_opts.aspect_w = opts.chart_params[1];
            chart_opts.aspect_h = opts.chart_params[2];
        }
        // Parse patches params if provided
        if (!opts.chart_patches_params.empty()) {
            chart_opts.patches_m = opts.chart_patches_params[0]; // Rows
            chart_opts.patches_n = opts.chart_patches_params[1]; // Cols
        }
    } catch (const std::exception& e) {
        log_stream << _("Error: Invalid parameter for a chart argument.") << std::endl;
        return std::nullopt;
    }
    
    return chart_opts;
}