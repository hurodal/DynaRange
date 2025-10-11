// File: src/core/utils/CommandGenerator.cpp
/**
 * @file src/core/utils/CommandGenerator.cpp
 * @brief Implements the CLI command string generator.
 */
#include "CommandGenerator.hpp"
#include "../arguments/ArgumentManager.hpp"
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
    std::stringstream command_ss;
    command_ss << DynaRange::Utils::Constants::CLI_EXECUTABLE_NAME;
    auto& mgr = ArgumentManager::Instance();

    auto add_arg = [&](const std::string& name) {
        bool use_short = (format == CommandFormat::PlotShort);
        if (!use_short) {
            command_ss << " --" << name;
            return;
        }
        // Map long names to short names for the short command format
        static const std::map<std::string, std::string> short_map
            = { { "black-level", " -B" }, { "black-file", " -b" }, { "saturation-level", " -S" }, { "saturation-file", " -s" }, { "input-files", " -i" }, { "patch-ratio", " -r" },
                  { "snrthreshold-db", " -d" }, { "drnormalization-mpx", " -m" }, { "poly-fit", " -f" }, { "output-file", " -o" }, { "plot-format", " -p" }, { "plot-params", " -P" },
                  { "print-patches", " -g" }, { "raw-channel", " -w" }, { "chart", " -c" }, { "chart-colour", " -C" }, { "chart-coords", " -x" }, { "chart-patches", " -M" } };
        auto it = short_map.find(name);
        if (it != short_map.end()) {
            command_ss << it->second;
        } else {
            command_ss << " --" << name; // Fallback for args without a short version
        }
    };

    std::string black_file = mgr.Get<std::string>("black-file");
    if (!black_file.empty()) {
        add_arg("black-file");
        if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            command_ss << " \"" << black_file << "\"";
        } else {
            command_ss << " \"" << fs::path(black_file).filename().string() << "\"";
        }
    } else {
        add_arg("black-level");
        command_ss << " " << std::fixed << std::setprecision(2) << mgr.Get<double>("black-level");
    }

    std::string sat_file = mgr.Get<std::string>("saturation-file");
    if (!sat_file.empty()) {
        add_arg("saturation-file");
        if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            command_ss << " \"" << sat_file << "\"";
        } else {
            command_ss << " \"" << fs::path(sat_file).filename().string() << "\"";
        }
    } else {
        add_arg("saturation-level");
        command_ss << " " << std::fixed << std::setprecision(2) << mgr.Get<double>("saturation-level");
    }

    if (format == CommandFormat::Full) {
        add_arg("output-file");
        command_ss << " \"" << mgr.Get<std::string>("output-file") << "\"";
    }

    if (!mgr.Get<bool>("snr-threshold-is-default")) {
        add_arg("snrthreshold-db");
        const auto& thresholds = mgr.Get<std::vector<double>>("snrthreshold-db");
        for (const auto& threshold : thresholds) {
            command_ss << " " << threshold;
        }
    }

    add_arg("drnormalization-mpx");
    command_ss << " " << mgr.Get<double>("drnormalization-mpx");
    add_arg("poly-fit");
    command_ss << " " << mgr.Get<int>("poly-fit");
    add_arg("patch-ratio");
    command_ss << " " << mgr.Get<double>("patch-ratio");

    if (mgr.Get<bool>("generate-plot")) {
        add_arg("plot-format");
        command_ss << " " << mgr.Get<std::string>("plot-format");

        add_arg("plot-params");
        const auto& plot_params = mgr.Get<std::vector<int>>("plot-params");
        for (const auto& val : plot_params) {
            command_ss << " " << val;
        }
    }

    const auto& chart_coords = mgr.Get<std::vector<double>>("chart-coords");
    if (!chart_coords.empty()) {
        add_arg("chart-coords");
        for (const auto& coord : chart_coords)
            command_ss << " " << coord;
    }

    const auto& chart_patches = mgr.Get<std::vector<int>>("chart-patches");
    if (!chart_patches.empty()) {
        add_arg("chart-patches");
        for (const auto& val : chart_patches)
            command_ss << " " << val;
    }

    // Add --raw-channel to the command string if it's not the default.
    const auto& raw_channels_vec = mgr.Get<std::vector<int>>("raw-channel");
    const std::vector<int> default_channels = { 0, 0, 0, 0, 1 };
    if (raw_channels_vec != default_channels) {
        add_arg("raw-channel");
        for (const auto& val : raw_channels_vec) {
            command_ss << " " << val;
        }
    }

    if (format == CommandFormat::Full || format == CommandFormat::GuiPreview) {
        const auto& input_files = mgr.Get<std::vector<std::string>>("input-files");
        if (!input_files.empty()) {
            add_arg("input-files");
            for (const auto& file : input_files) {
                command_ss << " \"" << file << "\"";
            }
        }
    }

    return command_ss.str();
}

} // namespace CommandGenerator