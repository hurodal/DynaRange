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

    // Variables temporales para almacenar los valores de la línea de comandos
    std::string dark_file, sat_file;
    double dark_value_cli = -1.0, sat_value_cli = -1.0;

    // --- Grupo 1: Nivel de Negro (mutuamente excluyente y obligatorio) ---
    auto dark_group = app.add_option_group(_("Dark Frame"), _("Options for the black level"));
    auto opt_dark_file = dark_group->add_option("--dark-file", dark_file, _("RAW file to calculate the black level"))
                           ->check(CLI::ExistingFile);
    auto opt_dark_value = dark_group->add_option("--dark-value", dark_value_cli, _("Numeric value for the black level"))
                            ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    dark_group->require_option(1); 

    // --- Grupo 2: Nivel de Saturación (mutuamente excluyente y obligatorio) ---
    auto sat_group = app.add_option_group(_("Saturation"), _("Options for the saturation point"));
    auto opt_sat_file = sat_group->add_option("--sat-file", sat_file, _("RAW file to calculate the saturation"))
                          ->check(CLI::ExistingFile);
    auto opt_sat_value = sat_group->add_option("--sat-value", sat_value_cli, _("Numeric value for the saturation"))
                           ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    sat_group->require_option(1);

    // --- Opción 3: Fichero de salida (opcional) ---
    app.add_option("-o,--output-data", opts.output_filename, _("Output CSV file"))
       ->default_val("DR_results.csv");

    // --- Opción 4: Ficheros de entrada (obligatorio) ---
    app.add_option("-f,--files", opts.input_files, _("List of RAW files to process"))
       ->required()
       ->check(CLI::ExistingFile);

    // --- Gestión de errores y parseo ---
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // app.exit(e) imprime el mensaje de ayuda/error y devuelve un código de salida.
        // Aunque debería terminar el programa, en algunos entornos no lo hace.
        // Por eso, capturamos el código de salida y terminamos el programa manualmente.
        int exit_code = app.exit(e);
        exit(exit_code);
    }

    // --- Lógica post-parseo ---
    // Determinar el valor final de 'dark_value'
    if (*opt_dark_file) {
        opts.dark_value = process_dark_frame(dark_file);
    } else {
        opts.dark_value = dark_value_cli;
    }

    // Determinar el valor final de 'saturation_value'
    if (*opt_sat_file) {
        opts.saturation_value = process_saturation_frame(sat_file);
    } else {
        opts.saturation_value = sat_value_cli;
    }

    return opts;
}