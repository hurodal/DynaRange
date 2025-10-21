// File: src/core/utils/CommandGenerator.cpp
/**
 * @file src/core/utils/CommandGenerator.cpp
 * @brief Implements the CLI command string generator.
 */
#include "CommandGenerator.hpp"
#include "../arguments/ArgumentManager.hpp"
#include "../arguments/Constants.hpp"
#include "Constants.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <libintl.h>
#include <sstream>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace CommandGenerator {

std::string GenerateCommand(CommandFormat format)
{
    using namespace DynaRange::Arguments::Constants;
    std::stringstream command_ss;
    command_ss << DynaRange::Utils::Constants::CLI_EXECUTABLE_NAME;
    auto& mgr = ArgumentManager::Instance();

    // Helper lambda to add argument names (handles short/long format)
    auto add_arg = [&](const std::string& name) {
        bool use_short = (format == CommandFormat::PlotShort);
        if (!use_short) {
            command_ss << " --" << name;
            return;
        }

        static const std::map<std::string, std::string> short_map
            = { { BlackLevel, " -B" }, { BlackFile, " -b" }, { SaturationLevel, " -S" }, { SaturationFile, " -s" }, { InputFiles, " -i" }, { PatchRatio, " -r" },
                  { SnrThresholdDb, " -d" }, { DrNormalizationMpx, " -m" }, { PolyFit, " -f" }, { OutputFile, " -o" }, { PlotFormat, " -p" }, { PlotParams, " -P" },
                  { PrintPatches, " -g" }, { RawChannels, " -w" }, { Chart, " -c" }, { ChartColour, " -C" }, { ChartCoords, " -x" }, { ChartPatches, " -M" },
                  { FullDebug, " -D"} }; // Agrega la versión corta del flag

        auto it = short_map.find(name);
        if (it != short_map.end()) {
            command_ss << it->second;
        } else {
            command_ss << " --" << name;
        }
    };

    // --debug / -D SI ESTÁ ACTIVO ---
    if (mgr.Get<bool>(FullDebug)) {
        add_arg(FullDebug); // Añade --debug o -D
    }

    std::string black_file = mgr.Get<std::string>(BlackFile);
    if (!black_file.empty()) {
        add_arg(BlackFile);
        if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            command_ss << " \"" << black_file << "\"";
        } else {
            command_ss << " \"" << fs::path(black_file).filename().string() << "\"";
        }
    } else if (!mgr.Get<bool>(BlackLevelIsDefault)) { // Add -B only if not default
        add_arg(BlackLevel);
        command_ss << " " << std::fixed << std::setprecision(2) << mgr.Get<double>(BlackLevel);
    }

    std::string sat_file = mgr.Get<std::string>(SaturationFile);
    if (!sat_file.empty()) {
        add_arg(SaturationFile);
        if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            command_ss << " \"" << sat_file << "\"";
        } else {
            command_ss << " \"" << fs::path(sat_file).filename().string() << "\"";
        }
    } else if (!mgr.Get<bool>(SaturationLevelIsDefault)) { // Add -S only if not default
        add_arg(SaturationLevel);
        command_ss << " " << std::fixed << std::setprecision(2) << mgr.Get<double>(SaturationLevel);
    }

    // Output file argument (-o) is usually added only for full command format
    if (format == CommandFormat::Full) {
        std::string output_file = mgr.Get<std::string>(OutputFile);
        // Only add if it's not the default filename
        if (output_file != DEFAULT_OUTPUT_FILENAME) {
             add_arg(OutputFile);
             command_ss << " \"" << output_file << "\"";
        }
    }

    if (!mgr.Get<bool>(SnrThresholdIsDefault)) {
        add_arg(SnrThresholdDb);
        const auto& thresholds = mgr.Get<std::vector<double>>(SnrThresholdDb);
        for (const auto& threshold : thresholds) {
            // Format threshold nicely (remove trailing zeros if integer)
             double intpart;
             if (std::modf(threshold, &intpart) == 0.0) {
                 command_ss << " " << static_cast<int>(threshold);
             } else {
                 command_ss << " " << threshold;
             }
        }
    }

    // Add only if not default
    double dr_norm = mgr.Get<double>(DrNormalizationMpx);
    if (dr_norm != DEFAULT_DR_NORMALIZATION_MPX) {
        add_arg(DrNormalizationMpx);
        command_ss << " " << dr_norm;
    }

    // Add only if not default
    int poly_order = mgr.Get<int>(PolyFit);
    if (poly_order != DEFAULT_POLY_ORDER) {
        add_arg(PolyFit);
        command_ss << " " << poly_order;
    }

     // Add only if not default
    double patch_ratio = mgr.Get<double>(PatchRatio);
     if (patch_ratio != DEFAULT_PATCH_RATIO) {
        add_arg(PatchRatio);
        command_ss << " " << patch_ratio;
    }

    if (mgr.Get<bool>(GeneratePlot)) {
        // Add plot format only if not default (PNG)
        std::string plot_format = mgr.Get<std::string>(PlotFormat);
        std::string default_plot_format = "PNG"; // Assuming PNG is default from registry
        std::transform(plot_format.begin(), plot_format.end(), plot_format.begin(), ::toupper);
        if (plot_format != default_plot_format) {
            add_arg(PlotFormat);
            command_ss << " " << plot_format;
        }

        // Add plot params only if not default (1 1 1 3)
        const auto& plot_params = mgr.Get<std::vector<int>>(PlotParams);
        const std::vector<int> default_plot_params = {1, 1, 1, 3}; // Assuming default from registry
        if (plot_params != default_plot_params) {
            add_arg(PlotParams);
            for (const auto& val : plot_params) {
                command_ss << " " << val;
            }
        }
    }

    // Print patches argument (-g)
    std::string print_patches_file = mgr.Get<std::string>(PrintPatches);
    // Add if it has a value different from the internal sentinel default
    if (print_patches_file != "_USE_DEFAULT_PRINT_PATCHES_") {
         add_arg(PrintPatches);
         // If the value is empty, it means the flag was used without an argument (use default name)
         // If it has a value, use that specific filename.
         if (!print_patches_file.empty()) {
            if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
                 command_ss << " \"" << print_patches_file << "\"";
             } else {
                 command_ss << " \"" << fs::path(print_patches_file).filename().string() << "\"";
             }
         }
    }


    const auto& chart_coords = mgr.Get<std::vector<double>>(ChartCoords);
    if (!chart_coords.empty()) {
        add_arg(ChartCoords);
        for (const auto& coord : chart_coords)
            command_ss << " " << static_cast<int>(round(coord));
    }

    const auto& chart_patches = mgr.Get<std::vector<int>>(ChartPatches);
    const std::vector<int> default_chart_patches = {DEFAULT_CHART_PATCHES_M, DEFAULT_CHART_PATCHES_N};
    // Add only if not default
    if (chart_patches != default_chart_patches) {
        add_arg(ChartPatches);
        for (const auto& val : chart_patches)
            command_ss << " " << val;
    }

    const auto& raw_channels_vec = mgr.Get<std::vector<int>>(RawChannels);
    const std::vector<int> default_channels = { 0, 0, 0, 0, 1 };
    // Add only if not default
    if (raw_channels_vec != default_channels) {
        add_arg(RawChannels);
        for (const auto& val : raw_channels_vec) {
            command_ss << " " << val;
        }
    }

    // Input files are always last
    const auto& input_files = mgr.Get<std::vector<std::string>>(InputFiles);
    if (!input_files.empty()) {
        // Add -i only if it wasn't added automatically by the format choice
        if (format != CommandFormat::Full && format != CommandFormat::GuiPreview) {
             // Add -i for short/long plot formats if input files exist
             add_arg(InputFiles);
        } else if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            // Add --input-files for full/gui preview if files exist
            add_arg(InputFiles);
        }

        for (const auto& file : input_files) {
            if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
                command_ss << " \"" << file << "\"";
            } else {
                command_ss << " \"" << fs::path(file).filename().string() << "\"";
            }
        }
    }

    return command_ss.str();
}
} // namespace CommandGenerator