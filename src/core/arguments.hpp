// Fichero: core/Arguments.hpp
#pragma once

#include <string>
#include <vector>

// La estructura de opciones del programa.
struct ProgramOptions {
    double dark_value;
    double saturation_value;
    std::string dark_file_path;
    std::string sat_file_path;
    std::string output_filename;
    std::vector<std::string> input_files;
    int poly_order = 2;
};

// Declaración de la función para parsear argumentos, ahora en PascalCase.
ProgramOptions ParseArguments(int argc, char* argv[]);