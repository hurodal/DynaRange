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

    // Paths to the files (if used by the GUI)
    std::string dark_file_path;
    std::string sat_file_path;

    // Input and output files
    std::string output_filename; // Name of the CSV file where results will be saved.
    std::vector<std::string> input_files; // List of input files.
};

// Function responsible for parsing the command-line arguments using CLI11.
//
// Argument Rules:
// - --dark-file OR --dark-value: one and only one must be present.
// - --sat-file OR --sat-value: one and only one must be present.
// - --output-data,-o: optional; if not specified, a default value is used.
// - --files,-f: mandatory. List of raw files to process.
//
// @param argc: number of arguments passed to the program.
// @param argv: array of strings with the arguments.
// @return: ProgramOptions structure with the final configured values.
// @throws CLI::ParseError if the arguments are invalid.
ProgramOptions parse_arguments(int argc, char* argv[]);