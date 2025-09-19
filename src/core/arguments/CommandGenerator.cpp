// File: src/core/arguments/CommandGenerator.cpp
/**
 * @file src/core/arguments/CommandGenerator.cpp
 * @brief Implements the generation of command-line strings from ProgramOptions.
 * @details This is the sole implementation of the command generation logic.
 *          It depends only on the ProgramOptions definition and the filesystem
 *          for path manipulation. It has no knowledge of CLI11 or argument parsing.
 */
#include "CommandGenerator.hpp"
#include <sstream>
#include <iomanip>
#include <filesystem>

// Namespace alias for std::filesystem
namespace fs = std::filesystem;

std::string GenerateCommand(const ProgramOptions& opts, CommandFormat format) {
    std::stringstream command_ss;
    command_ss << "rango";

    // Black level options
    if (!opts.dark_file_path.empty()) {
        command_ss << (format == CommandFormat::Plot ? " --black-file \"" : " -B \"");
        if (format == CommandFormat::Plot) {
            command_ss << fs::path(opts.dark_file_path).filename().string();
        } else {
            command_ss << opts.dark_file_path;
        }
        command_ss << "\"";
    } else {
        command_ss << (format == CommandFormat::Plot ? " --black-level " : " -b ") << opts.dark_value;
    }

    // Saturation level options
    if (!opts.sat_file_path.empty()) {
        command_ss << (format == CommandFormat::Plot ? " --saturation-file \"" : " -S \"");
        if (format == CommandFormat::Plot) {
            command_ss << fs::path(opts.sat_file_path).filename().string();
        } else {
            command_ss << opts.sat_file_path;
        }
        command_ss << "\"";
    } else {
        command_ss << (format == CommandFormat::Plot ? " --saturation-level " : " -s ") << opts.saturation_value;
    }

    // Parameter options
    if (format == CommandFormat::Full) {
        command_ss << " -o \"" << opts.output_filename << "\"";
    }
    if (opts.snr_thresholds_db.size() == 1) {
        command_ss << (format == CommandFormat::Plot ? " --snrthreshold-db " : " -d ")
                   << std::fixed << std::setprecision(2) << opts.snr_thresholds_db[0];
    }
    command_ss << (format == CommandFormat::Plot ? " --drnormalization-mpx " : " -m ")
               << std::fixed << std::setprecision(2) << opts.dr_normalization_mpx;
    command_ss << (format == CommandFormat::Plot ? " --poly-fit " : " -f ") << opts.poly_order;
    command_ss << (format == CommandFormat::Plot ? " --patch-ratio " : " -r ")
               << std::fixed << std::setprecision(2) << opts.patch_ratio;
    command_ss << (format == CommandFormat::Plot ? " --plot " : " -p ") << opts.plot_mode;

    // The input file list is only added for the full format.
    if (format == CommandFormat::Full) {
        command_ss << " -i";
        for (const auto& file : opts.input_files) {
            command_ss << " \"" << file << "\"";
        }
    }

    return command_ss.str();
}