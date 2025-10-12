// File: src/core/arguments/ArgumentManager.cpp
/**
 * @file ArgumentManager.cpp
 * @brief Implements the centralized argument management system.
 */
#include "ArgumentManager.hpp"
#include "parsing/ArgumentRegistry.hpp"
#include "parsing/CliParser.hpp"
#include "parsing/OptionsConverter.hpp"
#include <libintl.h>

#define _(string) gettext(string)

ArgumentManager& ArgumentManager::Instance()
{
    static ArgumentManager instance;
    return instance;
}

ArgumentManager::ArgumentManager()
{
    // 1. Get all argument definitions from the registry.
    m_descriptors = DynaRange::Arguments::Parsing::ArgumentRegistry::RegisterAll();

    // 2. Populate the values map with defaults immediately upon creation.
    for (const auto& [name, desc] : m_descriptors) {
        m_values[name] = desc.default_value;
    }
}

void ArgumentManager::ParseCli(int argc, char* argv[])
{
    // 1. Instantiate the parser and parse the command line.
    DynaRange::Arguments::Parsing::CliParser parser;
    auto parsed_values = parser.Parse(argc, argv, m_descriptors);

    // 2. Merge the parsed values into the manager's state.
    // This overwrites defaults with any values provided by the user.
    for (const auto& [name, value] : parsed_values) {
        m_values[name] = value;
    }
}

ProgramOptions ArgumentManager::ToProgramOptions()
{
    // Delegate the conversion to the specialized converter.
    return DynaRange::Arguments::Parsing::OptionsConverter::ToProgramOptions(m_values);
}

void ArgumentManager::Set(const std::string& long_name, std::any value)
{
    if (m_descriptors.count(long_name)) {
        m_values[long_name] = std::move(value);
    }
}