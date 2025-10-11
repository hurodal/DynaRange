// File: src/core/utils/CommandGenerator.cpp
/**
 * @file src/core/utils/CommandGenerator.cpp
 * @brief Implements the CLI command string generator.
 */
#include "CommandGenerator.hpp"
#include "../arguments/ArgumentManager.hpp"
#include "../arguments/Constants.hpp"
#include "Constants.hpp"
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

    auto add_arg = [&](const std::string& name) {
        bool use_short = (format == CommandFormat::PlotShort);
        if (!use_short) {
            command_ss << " --" << name;
            return;
        }
        
        static const std::map<std::string, std::string> short_map
            = { { BlackLevel, " -B" }, { BlackFile, " -b" }, { SaturationLevel, " -S" }, { SaturationFile, " -s" }, { InputFiles, " -i" }, { PatchRatio, " -r" },
                  { SnrThresholdDb, " -d" }, { DrNormalizationMpx, " -m" }, { PolyFit, " -f" }, { OutputFile, " -o" }, { PlotFormat, " -p" }, { PlotParams, " -P" },
                  { PrintPatches, " -g" }, { RawChannels, " -w" }, { Chart, " -c" }, { ChartColour, " -C" }, { ChartCoords, " -x" }, { ChartPatches, " -M" } };
        auto it = short_map.find(name);
        if (it != short_map.end()) {
            command_ss << it->second;
        } else {
            command_ss << " --" << name; 
        }
    };

    std::string black_file = mgr.Get<std::string>(BlackFile);
    if (!black_file.empty()) {
        add_arg(BlackFile);
        if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            command_ss << " \"" << black_file << "\"";
        } else {
            command_ss << " \"" << fs::path(black_file).filename().string() << "\"";
        }
    } else {
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
    } else {
        add_arg(SaturationLevel);
        command_ss << " " << std::fixed << std::setprecision(2) << mgr.Get<double>(SaturationLevel);
    }

    if (format == CommandFormat::Full) {
        add_arg(OutputFile);
        command_ss << " \"" << mgr.Get<std::string>(OutputFile) << "\"";
    }

    if (!mgr.Get<bool>(SnrThresholdIsDefault)) {
        add_arg(SnrThresholdDb);
        const auto& thresholds = mgr.Get<std::vector<double>>(SnrThresholdDb);
        for (const auto& threshold : thresholds) {
            command_ss << " " << threshold;
        }
    }

    add_arg(DrNormalizationMpx);
    command_ss << " " << mgr.Get<double>(DrNormalizationMpx);
    add_arg(PolyFit);
    command_ss << " " << mgr.Get<int>(PolyFit);
    add_arg(PatchRatio);
    command_ss << " " << mgr.Get<double>(PatchRatio);

    if (mgr.Get<bool>(GeneratePlot)) {
        add_arg(PlotFormat);
        command_ss << " " << mgr.Get<std::string>(PlotFormat);

        add_arg(PlotParams);
        const auto& plot_params = mgr.Get<std::vector<int>>(PlotParams);
        for (const auto& val : plot_params) {
            command_ss << " " << val;
        }
    }

    const auto& chart_coords = mgr.Get<std::vector<double>>(ChartCoords);
    if (!chart_coords.empty()) {
        add_arg(ChartCoords);
        for (const auto& coord : chart_coords)
            command_ss << " " << coord;
    }

    const auto& chart_patches = mgr.Get<std::vector<int>>(ChartPatches);
    if (!chart_patches.empty()) {
        add_arg(ChartPatches);
        for (const auto& val : chart_patches)
            command_ss << " " << val;
    }

    const auto& raw_channels_vec = mgr.Get<std::vector<int>>(RawChannels);
    const std::vector<int> default_channels = { 0, 0, 0, 0, 1 };
    if (raw_channels_vec != default_channels) {
        add_arg(RawChannels);
        for (const auto& val : raw_channels_vec) {
            command_ss << " " << val;
        }
    }

    if (format == CommandFormat::Full || format == CommandFormat::GuiPreview) {
        const auto& input_files = mgr.Get<std::vector<std::string>>(InputFiles);
        if (!input_files.empty()) {
            add_arg(InputFiles);
            for (const auto& file : input_files) {
                command_ss << " \"" << file << "\"";
            }
        }
    }

    return command_ss.str();
}

} // namespace CommandGenerator