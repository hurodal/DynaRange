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
    // The struct is now created with all the correct default values already initialized.
    ChartGeneratorOptions chart_opts{};

    try {
        // The rest of the function now only overwrites the defaults if the user provided them.
        if (!opts.chart_colour_params.empty() && !opts.chart_colour_params[0].empty()) {
            if (opts.chart_colour_params.size() >= 1) chart_opts.R = std::stoi(opts.chart_colour_params[0]);
            if (opts.chart_colour_params.size() >= 2) chart_opts.G = std::stoi(opts.chart_colour_params[1]);
            if (opts.chart_colour_params.size() >= 3) chart_opts.B = std::stoi(opts.chart_colour_params[2]);
            if (opts.chart_colour_params.size() >= 4) chart_opts.invgamma = std::stod(opts.chart_colour_params[3]);
        }
        
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