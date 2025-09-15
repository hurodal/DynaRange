// File: src/core/arguments/CommandLineParser.hpp
/**
 * @file src/core/arguments/CommandLineParser.hpp
 * @brief Declares the function to parse command-line arguments into ProgramOptions.
 */
#pragma once

#include "Arguments.hpp"

/**
 * @brief Parses command-line arguments and returns a fully configured ProgramOptions object.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return A ProgramOptions struct populated with parsed values and defaults.
 */
ProgramOptions ParseCommandLine(int argc, char* argv[]);