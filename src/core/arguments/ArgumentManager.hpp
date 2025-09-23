// File: src/core/arguments/ArgumentManager.hpp
/**
 * @file ArgumentManager.hpp
 * @brief Declares a centralized manager for all program arguments.
 */
#pragma once
#include "ProgramOptions.hpp"
#include <string>
#include <any>
#include <optional>
#include <map>

enum class ArgType {
    Int, Double, String, StringVector, Flag
};

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
    std::string GenerateCommand(CommandFormat format = CommandFormat::Full);
    ProgramOptions ToProgramOptions();

    // Método genérico para establecer el valor de un argumento
    void Set(const std::string& long_name, std::any value);

    template<typename T>
    T Get(const std::string& long_name) const;

private:
    // Private constructor/destructor for the Singleton pattern.
    ArgumentManager(); //  Eliminado el '= default'
    ~ArgumentManager() = default;

    // Prohibit copying and assignment.
    ArgumentManager(const ArgumentManager&) = delete;
    ArgumentManager& operator=(const ArgumentManager&) = delete;

    void RegisterAllArguments();

    std::map<std::string, ArgumentDescriptor> m_descriptors;
    std::map<std::string, std::any> m_values;
    bool m_is_registered = false;
};