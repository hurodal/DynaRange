// File: src/core/arguments/CommandLineParser.hpp
/**
 * @file src/core/arguments/CommandLineParser.hpp
 * @brief Declares the function to parse command-line arguments into ProgramOptions.
 * @details This file has a single responsibility: to convert argc/argv into
 *          a populated ProgramOptions struct. It contains no logic for
 *          generating command strings.
 */
#pragma once
#include "ProgramOptions.hpp"

#ifdef _WIN32
#include <windows.h>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Función para expandir un único patrón de fichero en Windows.
void expand_single_wildcard(const std::string& pattern, std::vector<std::string>& expanded_files) {
    WIN32_FIND_DATAA find_data;
    HANDLE h_find = FindFirstFileA(pattern.c_str(), &find_data);

    if (h_find != INVALID_HANDLE_VALUE) {
        fs::path pattern_path(pattern);
        fs::path parent_dir = pattern_path.parent_path();
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                expanded_files.push_back((parent_dir / find_data.cFileName).string());
            }
        } while (FindNextFileA(h_find, &find_data) != 0);
        FindClose(h_find);
    }
}

// Función principal que procesa la lista de argumentos y expande los que contengan wildcards.
std::vector<std::string> expand_wildcards_on_windows(const std::vector<std::string>& files) {
    std::vector<std::string> result_files;
    for (const auto& file_arg : files) {
        // Un argumento es un patrón si contiene '*' o '?'.
        if (file_arg.find_first_of("*?") != std::string::npos) {
            expand_single_wildcard(file_arg, result_files);
        } else {
            // Si no tiene wildcards, se añade directamente.
            result_files.push_back(file_arg);
        }
    }
    return result_files;
}

#endif // _WIN32

/**
 * @brief Parses command-line arguments and returns a fully configured ProgramOptions object.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return A ProgramOptions struct populated with parsed values and defaults.
 */
ProgramOptions ParseCommandLine(int argc, char* argv[]);