// File: src/core/arguments/CommandGenerator.hpp
/**
 * @file src/core/arguments/CommandGenerator.hpp
 * @brief Declares the function to generate a command-line string from ProgramOptions.
 */
#pragma once

#include "Arguments.hpp"

/**
 * @brief Generates a formatted command-line string representation of the given options.
 * @param opts The program options to convert.
 * @param format The desired output format: Full (for CLI) or Plot (for GUI display).
 * @return A string containing the equivalent rango command.
 */
std::string GenerateCommand(const ProgramOptions& opts, CommandFormat format = CommandFormat::Full);