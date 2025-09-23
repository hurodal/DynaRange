// File: src/rango.cpp
/**
 * @file src/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "core/arguments/ArgumentManager.hpp"
#include "core/engine/Engine.hpp"      
#include <iostream>
#include <libintl.h>
#include <clocale>
#include <atomic> 

#define _(string) gettext(string)

int main(int argc, char* argv[]) {
    // 1. Initialize the localization system for translations
    setlocale(LC_ALL, "");
    bindtextdomain("dynaRange", "locale");
    textdomain("dynaRange");

    // 2. Set the numeric locale to "C" for consistent number parsing.
    std::setlocale(LC_NUMERIC, "C");

    // 3. Parse arguments using the new manager.
    ArgumentManager::Instance().ParseCli(argc, argv);
    
    // 4. Convert the parsed arguments to the structure expected by the engine.
    ProgramOptions opts = ArgumentManager::Instance().ToProgramOptions();
    
    std::atomic<bool> cancel_flag{false}; 
    ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, std::cout, cancel_flag);
    
    // Check for a critical error only if a plot was expected to be generated.
    if (opts.plot_mode != 0 && !report.summary_plot_path.has_value()) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    // --- FIN DE LA CORRECCIÓN ---
    
    return 0;
}