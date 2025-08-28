// core/arguments.hpp
#pragma once

#include <string>
#include <vector>

// Estructura que contiene todos los valores de configuración obtenidos a partir
// de los argumentos de línea de comandos.
struct ProgramOptions {
    double dark_value;           // Valor del nivel de negro (dark frame).
    double saturation_value;     // Valor del punto de saturación.
    std::string output_filename; // Nombre del archivo CSV donde se guardarán los resultados.
    std::vector<std::string> input_files; // Lista de ficheros de entrada.
};

// Función encargada de parsear los argumentos de línea de comandos usando CLI11.
//
// Reglas de argumentos:
// - --dark-file O --dark-value: uno y solo uno debe estar presente.
// - --sat-file O --sat-value: uno y solo uno debe estar presente.
// - --output-data: opcional; si no se especifica, se usa un valor por defecto.
//
// @param argc: número de argumentos pasados al programa.
// @param argv: array de cadenas con los argumentos.
// @return: estructura ProgramOptions con los valores finales configurados.
// @throws CLI::ParseError si los argumentos son inválidos.
ProgramOptions parse_arguments(int argc, char* argv[]);
