// File: src/core/arguments/ChartOptionsParser.cpp
/**
 * @file ChartOptionsParser.cpp
 * @brief Implements the chart options parser.
 */
#include "ChartOptionsParser.hpp"
#include <iostream>
#include <libintl.h>

#define _(string) gettext(string)

std::optional<ChartGeneratorOptions> ParseChartOptions(const ProgramOptions& opts, std::ostream& log_stream) {
    ChartGeneratorOptions chart_opts;
    // Set default values updated to match the latest R script.
    chart_opts.R = 162;
    chart_opts.G = 64;
    chart_opts.B = 104;
    chart_opts.invgamma = 1.4;
    chart_opts.dim_x = 1920;
    chart_opts.aspect_w = 3;
    chart_opts.aspect_h = 2;
    chart_opts.patches_m = 4; // Rows
    chart_opts.patches_n = 6; // Cols
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

        if (!opts.chart_patches.empty()) {
            chart_opts.patches_m = opts.chart_patches[0]; // Rows
            chart_opts.patches_n = opts.chart_patches[1]; // Cols
        }

    } catch (const std::exception& e) {
        log_stream << _("Error: Invalid parameter for a chart argument.") << std::endl;
        return std::nullopt;
    }
    return chart_opts;
}