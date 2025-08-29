// core/arguments.cpp
#include "arguments.hpp"
#include "functions.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <limits>

#include <libintl.h>
#define _(string) gettext(string)

ProgramOptions parse_arguments(int argc, char* argv[]) {
    ProgramOptions opts{};
    CLI::App app{_("Calculates the dynamic range from a series of RAW images.")};

    // Temporary variables to store command-line values
    std::string dark_file, sat_file;
    double dark_value_cli = -1.0, sat_value_cli = -1.0;

    // --- Group 1: Black Level (mutually exclusive and mandatory) ---
    auto dark_group = app.add_option_group(_("Dark Frame"), _("Options for the black level"));
    auto opt_dark_file = dark_group->add_option("--dark-file", dark_file, _("RAW file to calculate the black level"))
                           ->check(CLI::ExistingFile);
    auto opt_dark_value = dark_group->add_option("--dark-value", dark_value_cli, _("Numeric value for the black level"))
                            ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    dark_group->require_option(1); 

    // --- Group 2: Saturation Level (mutually exclusive and mandatory) ---
    auto sat_group = app.add_option_group(_("Saturation"), _("Options for the saturation point"));
    auto opt_sat_file = sat_group->add_option("--sat-file", sat_file, _("RAW file to calculate the saturation"))
                          ->check(CLI::ExistingFile);
    auto opt_sat_value = sat_group->add_option("--sat-value", sat_value_cli, _("Numeric value for the saturation"))
                           ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    sat_group->require_option(1);

    // --- Option 3: Output file (optional) ---
    app.add_option("-o,--output-data", opts.output_filename, _("Output CSV file"))
       ->default_val("DR_results.csv");

    // --- Option 4: Input files (mandatory) ---
    app.add_option("-f,--files", opts.input_files, _("List of RAW files to process"))
       ->required() // Makes this argument mandatory
       ->check(CLI::ExistingFile); // Checks that the files exist

    // --- Error handling and parsing ---
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // We use app.exit(e) so that CLI11 prints the correct message (help or error)
        // and terminates the program. This function calls exit() and does not return.
        app.exit(e);
    }

    // --- Post-parsing logic ---
    // Determine the final 'dark_value'
    if (*opt_dark_file) { // If the --dark-file option was used
        opts.dark_value = process_dark_frame(dark_file);
    } else {
        opts.dark_value = dark_value_cli;
    }

    // Determine the final 'saturation_value'
    if (*opt_sat_file) { // If the --sat-file option was used
        opts.saturation_value = process_saturation_frame(sat_file);
    } else {
        opts.saturation_value = sat_value_cli;
    }

    return opts;
}