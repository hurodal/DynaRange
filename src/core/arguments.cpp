// Fichero: core/Arguments.cpp
#include "Arguments.hpp"
#include "Analysis.hpp" // Depende de las funciones de análisis
#include <CLI/CLI.hpp>
#include <iostream>
#include <limits>
#include <clocale>
#include <libintl.h>

#define _(string) gettext(string)

ProgramOptions ParseArguments(int argc, char* argv[]) {
    char* current_locale = setlocale(LC_NUMERIC, nullptr);
    setlocale(LC_NUMERIC, "C");

    ProgramOptions opts{};
    CLI::App app{_("Calculates the dynamic range from a series of RAW images.")};

    std::string dark_file, sat_file;
    double dark_value_cli = -1.0, sat_value_cli = -1.0;

    auto dark_group = app.add_option_group(_("Dark Frame"), _("Options for the black level"));
    auto opt_dark_file = dark_group->add_option("--dark-file", dark_file, _("RAW file to calculate the black level"))->check(CLI::ExistingFile);
    auto opt_dark_value = dark_group->add_option("--dark-value", dark_value_cli, _("Numeric value for the black level"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    dark_group->require_option(1); 

    auto sat_group = app.add_option_group(_("Saturation"), _("Options for the saturation point"));
    auto opt_sat_file = sat_group->add_option("--sat-file", sat_file, _("RAW file to calculate the saturation"))->check(CLI::ExistingFile);
    auto opt_sat_value = sat_group->add_option("--sat-value", sat_value_cli, _("Numeric value for the saturation"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    sat_group->require_option(1);

    app.add_option("-o,--output-data", opts.output_filename, _("Output CSV file"))->default_val("DR_results.csv");
    app.add_option("--poly-fit", opts.poly_order, _("Polynomic order (default=2) to fit the SNR curve"))->check(CLI::IsMember({2, 3}))->default_val(2);
    app.add_option("-f,--files", opts.input_files, _("List of RAW files to process"))->required()->check(CLI::ExistingFile);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        setlocale(LC_NUMERIC, current_locale);
        exit(app.exit(e));
    }

    setlocale(LC_NUMERIC, current_locale);

    if (*opt_dark_file) {
        // Llamada a la función renombrada
        auto dark_val_opt = ProcessDarkFrame(dark_file, std::cout);
        if (!dark_val_opt) {
            std::cerr << "Fatal error: Could not process dark file: " << dark_file << ". Exiting." << std::endl;
            exit(1);
        }
        opts.dark_value = *dark_val_opt;
    } else {
        opts.dark_value = dark_value_cli;
    }

    if (*opt_sat_file) {
        // Llamada a la función renombrada
        auto sat_val_opt = ProcessSaturationFrame(sat_file, std::cout);
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