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
#include <filesystem>

#ifdef _WIN32
#include <windows.h> // Para manejo correcto del path con locales
#endif

#define _(string) gettext(string)
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    
    // 1. Initialize the localization system to respect environment variables.
    setlocale(LC_ALL, "");

    // 2. Calculate the absolute path to the 'locale' directory.
    // This makes the app portable and work when run from the build directory.
    fs::path exe_path;
    #ifdef _WIN32
        wchar_t path_buf[MAX_PATH];
        // GetModuleFileNameW(NULL, ...) obtiene la ruta del proceso actual.
        GetModuleFileNameW(NULL, path_buf, MAX_PATH);
        exe_path = path_buf;
    #else
        // El método original es suficiente para Linux.
        exe_path = argv[0];
    #endif
    fs::path locale_dir = exe_path.parent_path() / "locale";
    bindtextdomain("dynaRange", locale_dir.string().c_str());    

    // 3. Set the text domain.
    textdomain("dynaRange");

    // 4. Set the numeric locale to "C" for consistent number parsing.
    std::setlocale(LC_NUMERIC, "C");

    // 5. Parse arguments using the manager.
    ArgumentManager::Instance().ParseCli(argc, argv);
    
    // 6. Convert the parsed arguments to the structure expected by the engine.
    ProgramOptions opts = ArgumentManager::Instance().ToProgramOptions();
    
    std::atomic<bool> cancel_flag{false};
    ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, std::cout, cancel_flag);
    
    // Check for a critical error only if a plot was expected to be generated.
    if (opts.plot_mode != 0 && !report.summary_plot_path.has_value()) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    
    return 0;
}