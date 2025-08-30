// core/arguments.cpp
#include "arguments.hpp"
#include "functions.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <limits>
#include <optional>

// Gettext is NO LONGER USED in the core library
// #include <libintl.h>
// #define _(string) gettext(string)

ProgramOptions parse_arguments(int argc, char* argv[]) {
    ProgramOptions opts{};
    // All strings are now in English (the source language)
    CLI::App app{"Calculates the dynamic range from a series of RAW images."};

    // Temporary variables to store command-line values
    std::string dark_file, sat_file;
    double dark_value_cli = -1.0, sat_value_cli = -1.0;

    // --- Group 1: Black Level (mutually exclusive and mandatory) ---
    auto dark_group = app.add_option_group("Dark Frame", "Options for the black level");
    auto opt_dark_file = dark_group->add_option("--dark-file", dark_file, "RAW file to calculate the black level")
                           ->check(CLI::ExistingFile);
    auto opt_dark_value = dark_group->add_option("--dark-value", dark_value_cli, "Numeric value for the black level")
                            ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    dark_group->require_option(1); 

    // --- Group 2: Saturation Level (mutually exclusive and mandatory) ---
    auto sat_group = app.add_option_group("Saturation", "Options for the saturation point");
    auto opt_sat_file = sat_group->add_option("--sat-file", sat_file, "RAW file to calculate the saturation")
                          ->check(CLI::ExistingFile);
    auto opt_sat_value = sat_group->add_option("--sat-value", sat_value_cli, "Numeric value for the saturation")
                           ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    sat_group->require_option(1);

    // --- Option 3: Output file (optional) ---
    app.add_option("-o,--output-data", opts.output_filename, "Output CSV file")
       ->default_val("DR_results.csv");

    // --- Option 4: Input files (mandatory) ---
    app.add_option("-f,--files", opts.input_files, "List of RAW files to process")
       ->required()
       ->check(CLI::ExistingFile);

    // --- Error handling and parsing ---
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        int exit_code = app.exit(e);
        exit(exit_code);
    }

    // --- Post-parsing logic ---
    if (*opt_dark_file) {
        auto dark_val_opt = process_dark_frame(dark_file);
        if (!dark_val_opt) {
            std::cerr << "Fatal error: Could not process dark file: " << dark_file << ". Exiting." << std::endl;
            exit(1);
        }
        opts.dark_value = *dark_val_opt;
    } else {
        opts.dark_value = dark_value_cli;
    }

    if (*opt_sat_file) {
        auto sat_val_opt = process_saturation_frame(sat_file);
        if (!sat_val_opt) {
            std::cerr << "Fatal error: Could not process saturation file: " << sat_file << ". Exiting." << std::endl;
            exit(1);
        }
        opts.saturation_value = *sat_val_opt;
    } else {
        opts.saturation_value = sat_value_cli;
    }

    return opts;
}