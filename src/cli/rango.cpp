// File: src/rango.cpp
/**
 * @file src/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/arguments/ChartOptionsParser.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/utils/LocaleManager.hpp"
#include "../core/utils/PathManager.hpp"
#include "../core/graphics/ChartGenerator.hpp"
#include <iostream>
#include <libintl.h>
#include <clocale>
#include <atomic>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#define _(string) gettext(string)

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");

    // Use PathManager to determine the locale directory path.
    // An empty ProgramOptions is sufficient for the PathManager to find the app path.
    PathManager path_manager(ProgramOptions{});
    bindtextdomain("dynaRange", path_manager.GetLocaleDirectory().string().c_str());
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
        // The call to GenerateTestChart is corrected to pass the options struct directly.
        if (!GenerateTestChart(chart_opts, chart_output_path.string(), std::cout)) {
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