// File: src/core/arguments/parsing/OptionsConverter.hpp
/**
 * @file OptionsConverter.hpp
 * @brief Declares a converter from parsed argument values to ProgramOptions.
 * @details This module adheres to SRP by encapsulating the logic of converting
 * a generic map of parsed values into the strongly-typed ProgramOptions
 * struct used by the core application.
 */
#pragma once

#include "../ArgumentsOptions.hpp"
#include <map>
#include <string>
#include <any>

namespace DynaRange::Arguments::Parsing {

/**
 * @class OptionsConverter
 * @brief Converts a map of parsed argument values into a ProgramOptions struct.
 */
class OptionsConverter {
public:
    /**
     * @brief Converts the map of parsed values to a ProgramOptions struct.
     * @param values The map of parsed values from the CliParser.
     * @return A populated ProgramOptions struct.
     */
    static ProgramOptions ToProgramOptions(const std::map<std::string, std::any>& values);
};

} // namespace DynaRange::Arguments::Parsing