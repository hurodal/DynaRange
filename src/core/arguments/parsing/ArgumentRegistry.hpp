// File: src/core/arguments/parsing/ArgumentRegistry.hpp
/**
 * @file ArgumentRegistry.hpp
 * @brief Declares the argument registry for the application.
 * @details This module adheres to SRP by centralizing the definition of all
 * command-line and program arguments, separating their registration from
 * parsing and conversion logic.
 */
#pragma once

#include "../ArgumentManager.hpp" // For ArgumentDescriptor
#include <map>
#include <string>

namespace DynaRange::Arguments::Parsing {

/**
 * @class ArgumentRegistry
 * @brief A static class responsible for defining all program arguments.
 */
class ArgumentRegistry {
public:
    /**
     * @brief Registers and returns descriptors for all application arguments.
     * @return A map where the key is the argument's long name and the value is its descriptor.
     */
    static std::map<std::string, ArgumentDescriptor> RegisterAll();
};

} // namespace DynaRange::Arguments::Parsing