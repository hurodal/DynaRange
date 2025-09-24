// File: src/rango.cpp
/**
 * @file src/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "core/arguments/ArgumentManager.hpp"
#include "core/engine/Engine.hpp"
#include "core/utils/LocaleManager.hpp"
#include "core/utils/PathManager.hpp"
#include "core/graphics/ChartGenerator.hpp"
#include <iostream>
#include <libintl.h>
#include <clocale>
#include <atomic>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#define _(string) gettext(string)

// File: src/rango.cpp

// This is the main function. It already existed and is now updated.
int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");
    std::filesystem::path exe_path = argv[0];
    std::filesystem::path locale_dir = exe_path.parent_path() / "locale";
    bindtextdomain("dynaRange", locale_dir.string().c_str());
    textdomain("dynaRange");
    
    LocaleManager locale_manager;

    ArgumentManager::Instance().ParseCli(argc, argv);
    ProgramOptions opts = ArgumentManager::Instance().ToProgramOptions();

    // Check for chart creation mode as the primary action
    if (opts.create_chart_mode) {
        // Parse optional parameters with defaults
        int R = 255, G = 101, B = 164;
        double invgamma = 1.4;
        try {
            if (opts.chart_params.size() >= 1) R = std::stoi(opts.chart_params[0]);
            if (opts.chart_params.size() >= 2) G = std::stoi(opts.chart_params[1]);
            if (opts.chart_params.size() >= 3) B = std::stoi(opts.chart_params[2]);
            if (opts.chart_params.size() >= 4) invgamma = std::stod(opts.chart_params[3]);
        } catch (const std::exception& e) {
            std::cerr << _("Error: Invalid parameter for --chart. Must be <R G B invgamma>.") << std::endl;
            return 1;
        }

        // Use PathManager to determine the correct output directory, ensuring consistency.
        PathManager paths(opts);
        fs::path chart_output_path = paths.GetCsvOutputPath().parent_path() / "magentachart.png";

        if (!GenerateTestChart(chart_output_path.string(), R, G, B, invgamma, std::cout)) {
            return 1;
        }
        return 0; // Exit successfully after chart generation
    }

    // Normal analysis mode logic continues here if --chart was not used
    std::atomic<bool> cancel_flag{false};
    ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, std::cout, cancel_flag);
    
    if (!report.summary_plot_path.has_value() && opts.plot_mode > 0) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }

    return 0;
}