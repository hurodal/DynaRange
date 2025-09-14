/**
 * @file src/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "core/Arguments.hpp"
#include "core/Engine.hpp"
#include <iostream>
#include <libintl.h>
#include <locale.h>

#define _(string) gettext(string)

/**
 * @brief The main function for the dynaRange CLI tool.
 * @details Initializes localization, parses command-line arguments,
 * runs the core analysis engine, and reports errors.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char* argv[]) {
    // Initialize localization settings for gettext
    setlocale(LC_ALL, "");
    bindtextdomain("dynaRange", "locale");
    textdomain("dynaRange");

    // 1. Parse arguments
    ProgramOptions opts = ParseArguments(argc, argv);
    
    // 2. Call the engine with the console (std::cout) as the log stream
    if (!RunDynamicRangeAnalysis(opts, std::cout)) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    
    return 0;
}