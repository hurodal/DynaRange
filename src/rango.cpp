// File: src/rango.cpp
/**
 * @file src/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "core/arguments/ArgumentManager.hpp"
#include "core/arguments/ChartOptionsParser.hpp"
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

    if (opts.create_chart_mode) {
        // La responsabilidad de interpretar los parámetros se delega al parser.
        auto chart_options_opt = ParseChartOptions(opts, std::cerr);
        if (!chart_options_opt) {
            return 1; // El parser ya ha informado del error.
        }
        
        // Se usan las opciones ya validadas y parseadas.
        const auto& chart_opts = *chart_options_opt;

        PathManager paths(opts);
        fs::path chart_output_path = paths.GetCsvOutputPath().parent_path() / "magentachart.png";

        if (!GenerateTestChart(chart_output_path.string(), 
                               chart_opts.R, chart_opts.G, chart_opts.B, chart_opts.invgamma, 
                               chart_opts.dim_x, chart_opts.aspect_w, chart_opts.aspect_h, 
                               chart_opts.patches_m, chart_opts.patches_n, 
                               std::cout)) {
            return 1;
        }
        return 0; // Se finaliza la ejecución tras generar la carta.
    }

    // La lógica del modo de análisis normal continúa aquí.
    std::atomic<bool> cancel_flag{false};
    ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, std::cout, cancel_flag);
    
    if (!report.summary_plot_path.has_value() && opts.plot_mode > 0) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }

    return 0;
}