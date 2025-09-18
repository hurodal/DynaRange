// File: src/core/arguments/CommandGenerator.hpp
/**
 * @file src/core/arguments/CommandGenerator.hpp
 * @brief Declares the function to generate a command-line string from ProgramOptions.
 * @details This file has a single responsibility: to convert a ProgramOptions
 *          struct into a human-readable or machine-executable command string.
 *          It contains no logic for parsing argc/argv.
 */
#pragma once
#include "ProgramOptions.hpp"

/**
 * @brief Generates a formatted command-line string representation of the given options.
 * @param opts The program options to convert.
 * @param format The desired output format: Full (for CLI) or Plot (for GUI display).
 * @return A string containing the equivalent rango command.
 */
std::string GenerateCommand(const ProgramOptions& opts, CommandFormat format = CommandFormat::Full);