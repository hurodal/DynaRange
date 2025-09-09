// File: core/Arguments.hpp
#pragma once

#include <string>
#include <vector>
#include <ostream>

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
    
    // New members for CLI arguments
    double snr_threshold_db;
    double dr_normalization_mpx;
    int patch_safe;

    // Flag and string for the command in the report
    bool report_command = false;
    std::string generated_command;
};

// Function declaration for parsing arguments.
ProgramOptions ParseArguments(int argc, char* argv[]);

// ADDED: Function declaration to generate the equivalent command line string.
std::string GenerateCommandString(const ProgramOptions& opts);