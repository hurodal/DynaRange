// File: src/core/arguments/ArgumentManager.hpp
/**
 * @file ArgumentManager.hpp
 * @brief Declares a centralized manager for all program arguments.
 */
#pragma once
#include "ArgumentsOptions.hpp"
#include <any>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>

enum class ArgType { Int, Double, String, StringVector, IntVector, DoubleVector, Flag };

struct ArgumentDescriptor {
    std::string long_name;
    std::string short_name;
    std::string help_text;
    ArgType type;
    std::any default_value;
    bool is_required = false;
    std::optional<std::any> min_value;
    std::optional<std::any> max_value;
};

class ArgumentManager {
public:
    static ArgumentManager& Instance();

    void ParseCli(int argc, char* argv[]);
    ProgramOptions ToProgramOptions();
    void Set(const std::string& long_name, std::any value);

    template <typename T> T Get(const std::string& long_name) const
    {
        if (m_values.count(long_name)) {
            try {
                return std::any_cast<T>(m_values.at(long_name));
            } catch (const std::bad_any_cast& e) {
                throw std::runtime_error("Invalid type requested for argument: " + long_name);
            }
        }
        throw std::runtime_error("Argument not found: " + long_name);
    }

private:
    ArgumentManager();
    ~ArgumentManager() = default;
    ArgumentManager(const ArgumentManager&) = delete;
    ArgumentManager& operator=(const ArgumentManager&) = delete;

    std::map<std::string, ArgumentDescriptor> m_descriptors;
    std::map<std::string, std::any> m_values;
};