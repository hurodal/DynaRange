// File: src/core/arguments/parsing/CliParser.hpp
/**
 * @file CliParser.hpp
 * @brief Declares a parser for command-line arguments.
 * @details This module adheres to SRP by encapsulating the logic of parsing
 * CLI arguments using the CLI11 library, separating it from argument
 * definition and conversion.
 */
#pragma once

#include "../ArgumentManager.hpp" // For ArgumentDescriptor
#include <map>
#include <string>
#include <any>

namespace DynaRange::Arguments::Parsing {

/**
 * @class CliParser
 * @brief Parses command-line arguments based on provided descriptors.
 */
class CliParser {
public:
    /**
     * @brief Parses the command-line arguments.
     * @param argc The argument count from main().
     * @param argv The argument vector from main().
     * @param descriptors A map of argument descriptors to configure the parser.
     * @return A map where the key is the argument's long name and the value is the parsed std::any value.
     */
    std::map<std::string, std::any> Parse(int argc, char* argv[], const std::map<std::string, ArgumentDescriptor>& descriptors);
};

} // namespace DynaRange::Arguments::Parsing