// File: src/rango.cpp
/**
 * @file src/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "core/arguments/CommandLineParser.hpp"  
#include "core/engine/Engine.hpp"      
#include <iostream>
#include <libintl.h>
#include <locale.h>
#include <atomic> 

#define _(string) gettext(string)

/**
 * @brief The main function for the dynaRange CLI tool.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return 0 on success, 1 on failure.
 */
int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");
    bindtextdomain("dynaRange", "locale");
    textdomain("dynaRange");

    ProgramOptions opts = ParseCommandLine(argc, argv);
    
    // 2. Crear un flag de cancelación que siempre será falso para el CLI.
    std::atomic<bool> cancel_flag{false}; 
    
    // 3. Pasar el flag a la función.
    ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, std::cout, cancel_flag);
    
    // Check for failure by seeing if the summary plot path was generated.
    if (!report.summary_plot_path.has_value()) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    
    return 0;
}