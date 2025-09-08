// Fichero: core/Arguments.hpp
#pragma once

#include <string>
#include <vector>

// Orden por defecto de los polinomios
constexpr int DEFAULT_POLY_ORDER = 3;

// La estructura de opciones del programa.
struct ProgramOptions {
    double dark_value;
    double saturation_value;
    std::string dark_file_path;
    std::string sat_file_path;
    std::string output_filename;
    std::vector<std::string> input_files;
    int poly_order = DEFAULT_POLY_ORDER;;
    
    // Nuevos miembros para los argumentos de la CLI
    std::string chart_colors;
    double snr_threshold_db;
    double dr_normalization_mpx;
    int patch_safe;
};

// Declaración de la función para parsear argumentos, ahora en PascalCase.
ProgramOptions ParseArguments(int argc, char* argv[]);