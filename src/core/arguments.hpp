// File: core/Arguments.hpp
#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <optional>

// Default polynomial order
constexpr int DEFAULT_POLY_ORDER = 3;

// Enum to specify the desired command string format
enum class CommandFormat {
    Full, // Complete command for the GUI
    Plot  // Abbreviated command for plots
};

// Structure for program options.
struct ProgramOptions {
    double dark_value;
    double saturation_value;
    std::string dark_file_path;
    std::string sat_file_path;
    std::string output_filename;
    std::vector<std::string> input_files;
    int poly_order = DEFAULT_POLY_ORDER;
    
    double dr_normalization_mpx;
    std::vector<double> snr_thresholds_db;

    double patch_ratio;
    int plot_mode;

    bool create_chart_mode = false;
    std::vector<double> chart_params;

    std::string generated_command;
};

// Function declaration for parsing arguments.
ProgramOptions ParseArguments(int argc, char* argv[]);

// The function now accepts a CommandFormat enum
std::string GenerateCommandString(const ProgramOptions& opts, CommandFormat format = CommandFormat::Full);