// File: src/core/utils/CommandGenerator.hpp
/**
 * @file src/core/utils/CommandGenerator.hpp
 * @brief Declares the function for generating the equivalent CLI command string.
 * @details This module's single responsibility is to construct a formatted
 * command string based on the current application state, separating this
 * logic from the argument parsing module.
 */
#pragma once

#include "../arguments/ProgramOptions.hpp"
#include <string>

namespace CommandGenerator {

/**
 * @brief Generates a string representing the equivalent command-line execution.
 * @param format The desired output format (e.g., full paths, short names).
 * @return The formatted command string.
 */
std::string GenerateCommand(CommandFormat format = CommandFormat::Full);

} // namespace CommandGenerator