// core/arguments.cpp
#include "arguments.hpp"
#include "functions.hpp"
#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <limits>

ProgramOptions parse_arguments(int argc, char* argv[]) {
    ProgramOptions opts{};
    CLI::App app{"Calcula el rango dinámico a partir de una serie de imágenes RAW."};

    // Variables temporales para almacenar los valores de la línea de comandos
    std::string dark_file, sat_file;
    double dark_value_cli = -1.0, sat_value_cli = -1.0;

    // --- Grupo 1: Nivel de Negro (mutuamente excluyente y obligatorio) ---
    auto dark_group = app.add_option_group("Dark Frame", "Opciones para el nivel de negro");
    auto opt_dark_file = dark_group->add_option("--df,--dark-file", dark_file, "Fichero RAW para calcular el nivel de negro")
                           ->check(CLI::ExistingFile);
    auto opt_dark_value = dark_group->add_option("--dv,--dark-value", dark_value_cli, "Valor numérico para el nivel de negro")
                            ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    dark_group->require_option(1); 

    // --- Grupo 2: Nivel de Saturación (mutuamente excluyente y obligatorio) ---
    auto sat_group = app.add_option_group("Saturation", "Opciones para el punto de saturación");
    auto opt_sat_file = sat_group->add_option("--sf,--sat-file", sat_file, "Fichero RAW para calcular la saturación")
                          ->check(CLI::ExistingFile);
    auto opt_sat_value = sat_group->add_option("--sv,--sat-value", sat_value_cli, "Valor numérico para la saturación")
                           ->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    sat_group->require_option(1);

    // --- Opción 3: Fichero de salida (opcional) ---
    app.add_option("-o,--output-data", opts.output_filename, "Fichero CSV de salida")
       ->default_val("DR_results.csv");

    // --- Opción 4: Ficheros de entrada (obligatorio) ---
    app.add_option("-f,--files", opts.input_files, "Lista de ficheros RAW a procesar")
       ->required() // Hace que este argumento sea obligatorio
       ->check(CLI::ExistingFile); // Comprueba que los ficheros existen

    // --- Gestión de errores y parseo ---
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        // Usamos app.exit(e) para que CLI11 imprima el mensaje correcto (ayuda o error)
        // y termine el programa. Esta función llama a exit() y no retorna.
        app.exit(e);
    }

    // --- Lógica post-parseo ---
    // Determinar el valor final de 'dark_value'
    if (*opt_dark_file) { // Si se usó la opción --dark-file
        opts.dark_value = process_dark_frame(dark_file);
    } else {
        opts.dark_value = dark_value_cli;
    }

    // Determinar el valor final de 'saturation_value'
    if (*opt_sat_file) { // Si se usó la opción --sat-file
        opts.saturation_value = process_saturation_frame(sat_file);
    } else {
        opts.saturation_value = sat_value_cli;
    }

    return opts;
}
