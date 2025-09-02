// core/arguments.hpp
#pragma once

#include <string>
#include <vector>

// Structure containing all configuration values obtained from
// the command-line arguments.
struct ProgramOptions {
    // Final numeric values
    double dark_value;           // Black level value (dark frame).
    double saturation_value;     // Saturation point value.

    // Paths to the files (if used)
    std::string dark_file_path;
    std::string sat_file_path;

    // Input and output files
    std::string output_filename; // Name of the CSV file where results will be saved.
    std::vector<std::string> input_files; // List of input files.
};

// Function responsible for parsing the command-line arguments using CLI11.
ProgramOptions parse_arguments(int argc, char* argv[]);