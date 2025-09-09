// File: core/Arguments.hpp
#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <optional>

// Default polynomial order
constexpr int DEFAULT_POLY_ORDER = 3;

// Structure for program options.
struct ProgramOptions {
    double dark_value;
    double saturation_value;
    std::string dark_file_path;
    std::string sat_file_path;
    std::string output_filename;
    std::vector<std::string> input_files;
    int poly_order = DEFAULT_POLY_ORDER;
    
    // MODIFIED: Arguments adapted to new specifications
    double dr_normalization_mpx;
    std::vector<double> snr_thresholds_db; // Can be {12, 0} by default, or a single user value.

    // ADDED: New arguments from the user manual
    double patch_ratio; // Replaces patch_safe
    int plot_mode;      // Replaces report_command

    // ADDED: New operating mode for chart creation (functionality not implemented yet)
    bool create_chart_mode = false;
    std::vector<double> chart_params;

    // REMOVED: Obsolete arguments
    // int patch_safe;
    // bool report_command;

    // This member is still used internally by the GUI
    std::string generated_command;
};

// Function declaration for parsing arguments.
ProgramOptions ParseArguments(int argc, char* argv[]);

// Function declaration to generate the equivalent command line string.
std::string GenerateCommandString(const ProgramOptions& opts);