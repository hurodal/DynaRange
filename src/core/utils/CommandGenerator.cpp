// File: src/core/utils/CommandGenerator.cpp
/**
 * @file src/core/utils/CommandGenerator.cpp
 * @brief Implements the CLI command string generator.
 */
#include "CommandGenerator.hpp"
#include "Constants.hpp" 
#include "../arguments/ArgumentManager.hpp"
#include <iomanip>
#include <sstream>
#include <libintl.h>
#include <filesystem>

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
        if (name == "poly-fit") command_ss << (use_short ? " -f" : " --poly-fit");
        else if (name == "patch-ratio") command_ss << (use_short ? " -r" : " --patch-ratio");
        else command_ss << " --" << name;
    };
    
    std::string black_file = mgr.Get<std::string>("black-file");
    if (!black_file.empty()) {
        if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            command_ss << " --black-file \"" << black_file << "\"";
        } else {
            command_ss << " --black-file \"" << fs::path(black_file).filename().string() << "\"";
        }
    } else {
        command_ss << " --black-level " << std::fixed << std::setprecision(2) << mgr.Get<double>("black-level");
    }

    std::string sat_file = mgr.Get<std::string>("saturation-file");
    if (!sat_file.empty()) {
        if (format == CommandFormat::GuiPreview || format == CommandFormat::Full) {
            command_ss << " --saturation-file \"" << sat_file << "\"";
        } else {
            command_ss << " --saturation-file \"" << fs::path(sat_file).filename().string() << "\"";
        }
    } else {
        command_ss << " --saturation-level " << std::fixed << std::setprecision(2) << mgr.Get<double>("saturation-level");
    }

    if (format == CommandFormat::Full) {
        command_ss << " --output-file \"" << mgr.Get<std::string>("output-file") << "\"";
    }

    if (!mgr.Get<bool>("snr-threshold-is-default")) {
        command_ss << " --snrthreshold-db " << mgr.Get<double>("snrthreshold-db");
    }

    command_ss << " --drnormalization-mpx " << mgr.Get<double>("drnormalization-mpx");
    command_ss << " --poly-fit " << mgr.Get<int>("poly-fit");
    command_ss << " --patch-ratio " << mgr.Get<double>("patch-ratio");
    command_ss << " --plot " << mgr.Get<int>("plot");

    // The --print-patches argument is a debug tool and will no longer be shown
    // in the equivalent command string to keep it clean.
    /*
    const auto& print_patches_file = mgr.Get<std::string>("print-patches");
    if (!print_patches_file.empty()) {
        command_ss << " --print-patches \"" << print_patches_file << "\"";
    }
    */

    const auto& chart_coords = mgr.Get<std::vector<double>>("chart-coords");
    if (!chart_coords.empty()) {
        command_ss << " --chart-coords";
        for (const auto& coord : chart_coords) command_ss << " " << coord;
    }

    const auto& chart_patches = mgr.Get<std::vector<int>>("chart-patches");
    if (!chart_patches.empty()) {
        command_ss << " --chart-patches";
        for (const auto& val : chart_patches) command_ss << " " << val;
    }

    // Add --raw-channel to the command string if it's not the default.
    const auto& raw_channels_vec = mgr.Get<std::vector<int>>("raw-channel");
    const std::vector<int> default_channels = {0, 0, 0, 0, 1};
    if (raw_channels_vec != default_channels) {
        command_ss << " --raw-channel";
        for (const auto& val : raw_channels_vec) {
            command_ss << " " << val;
        }
    }

    if (format == CommandFormat::Full || format == CommandFormat::GuiPreview) {
        const auto& input_files = mgr.Get<std::vector<std::string>>("input-files");
        if (!input_files.empty()) {
            command_ss << " --input-files";
            for (const auto& file : input_files) {
                command_ss << " \"" << file << "\"";
            }
        }
    }

    return command_ss.str();
}

} // namespace CommandGenerator