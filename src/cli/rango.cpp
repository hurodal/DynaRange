// File: src/cli/rango.cpp
/**
 * @file src/cli/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/arguments/ChartOptionsParser.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/utils/LocaleManager.hpp"
#include "../core/utils/PathManager.hpp"
#include "../core/graphics/ChartGenerator.hpp"
#include "../core/setup/PreAnalysisManager.hpp"
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
    PathManager path_manager(ProgramOptions{});
    bindtextdomain("dynaRange", path_manager.GetLocaleDirectory().string().c_str());
    textdomain("dynaRange");
    LocaleManager locale_manager;
    ArgumentManager::Instance().ParseCli(argc, argv);
    ProgramOptions opts = ArgumentManager::Instance().ToProgramOptions();
    if (opts.create_chart_mode) {
        auto chart_options_opt = ParseChartOptions(opts, std::cerr);
        if (!chart_options_opt) {
            return 1;
        }
        const auto& chart_opts = *chart_options_opt;
        PathManager paths(opts);
        fs::path chart_output_path = paths.GetCsvOutputPath().parent_path() / "magentachart.png";
        if (!GenerateTestChart(chart_opts, chart_output_path.string(), std::cout)) {
            return 1;
        }
        return 0;
    }
    if (!opts.input_files.empty()) {
        std::cout << _("Pre-analyzing files to extract metadata...") << std::endl;
        double sat_value = opts.saturation_level_is_default ? DEFAULT_SATURATION_LEVEL : opts.saturation_value;
        
        PreAnalysisManager pre_analysis_manager;
        for (const auto& file : opts.input_files) {
            pre_analysis_manager.AddFile(file, sat_value);
        }

        auto pre_analysis_results = pre_analysis_manager.GetSortedFileList();
        if (pre_analysis_results.empty()) {
            std::cerr << _("Error: None of the input files could be processed.") << std::endl;
            return 1;
        }

        auto best_file = pre_analysis_manager.GetBestPreviewFile();
        if (best_file.has_value()) {
            std::cout << _("Selected source file for corner detection: ") << best_file.value() << std::endl;
        }
    }
    std::atomic<bool> cancel_flag{false};
    ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, std::cout, cancel_flag);
    if (!report.summary_plot_path.has_value() && opts.generate_plot) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    return 0;
}