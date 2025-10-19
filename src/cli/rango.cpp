// File: src/cli/rango.cpp
/**
 * @file src/cli/rango.cpp
 * @brief Main entry point for the command-line (CLI) version of the application.
 */
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/arguments/ChartOptionsParser.hpp"
#include "../core/arguments/ArgumentsOptions.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/utils/LocaleManager.hpp"
#include "../core/utils/PathManager.hpp"
#include "../core/utils/OutputNamingContext.hpp"
#include "../core/artifacts/ArtifactFactory.hpp"
#include <iostream>
#include <libintl.h>
#include <clocale>
#include <atomic>
#include <filesystem>
#include <string>
#include <optional> // For std::optional

#ifdef _WIN32
#include <windows.h>
#endif

#define _(string) gettext(string)

namespace fs = std::filesystem;

/**
 * @brief Main function for the 'rango' CLI application.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, 1 on error.
 */
int main(int argc, char* argv[]) {
    // Set locale from environment for messages
    setlocale(LC_ALL, "");
    // Initialize PathManager early to find locale files
    PathManager path_manager_init(ProgramOptions{});
    // Use default opts for init path finding
    // Set up internationalization using gettext
    bindtextdomain("dynaRange", path_manager_init.GetLocaleDirectory().string().c_str());
    textdomain("dynaRange");
    // Set LC_NUMERIC to "C" for consistent number parsing/formatting
    LocaleManager locale_manager; // RAII class ensures restoration on exit

    // Parse command line arguments using the ArgumentManager singleton
    ArgumentManager::Instance().ParseCli(argc, argv);
    // Convert parsed arguments into the ProgramOptions struct
    ProgramOptions opts = ArgumentManager::Instance().ToProgramOptions();
    // Check if chart generation mode is requested
    if (opts.create_chart_mode) {
        // Parse the specific options required for chart generation
        std::optional<ChartGeneratorOptions> chart_options_opt = ParseChartOptions(opts, std::cerr);
        if (!chart_options_opt) {
            std::cerr << _("Error: Invalid chart generation parameters.") << std::endl;
            return 1; // Exit if chart options are invalid
        }
        const ChartGeneratorOptions& chart_opts = *chart_options_opt;

        // Create PathManager based on final options (e.g., output path hint)
        PathManager paths(opts);

        // Create context for Factory
        OutputNamingContext naming_ctx_chart;
        
        // Determine effective camera name based on opts (will use CLI defaults)
        std::string effective_name = "";
        if (opts.gui_use_camera_suffix) { // This will be false for CLI default
            if (opts.gui_use_exif_camera_name) {
                // EXIF name isn't available here, but logic is preserved
                effective_name = ""; // Placeholder, camera_name_exif not known here
            } else {
                effective_name = opts.gui_manual_camera_name; // Will be "" for CLI default
            }
        }
        naming_ctx_chart.effective_camera_name_for_output = effective_name;

        // Use ArtifactFactory to create and save the chart
        std::optional<fs::path> chart_path_opt = ArtifactFactory::CreateTestChartImage(
            chart_opts,
            naming_ctx_chart,
            paths,
            std::cout // Use cout for logging in CLI main
        );

        if (!chart_path_opt) {
            std::cerr << _("Error: Failed to generate or save test chart.") << std::endl; // Log message reused
            return 1; // Exit if generation fails
        }
        
        std::cout << _("Test chart generated successfully.") << std::endl; // General success message remains
        return 0; // Success
    }

    // --- Standard Analysis Mode ---
    // Initialize cancellation flag (not used interactively in CLI, but required by core function)
    std::atomic<bool> cancel_flag{false};
    // Run the main dynamic range analysis workflow
    // Note: RunDynamicRangeAnalysis internally handles filename generation now
    ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, std::cout, cancel_flag);
    // Check if analysis completed successfully, especially if plots were requested
    if (!report.summary_plot_path.has_value() && opts.generate_plot) {
        // Report might be empty due to error or cancellation
        if (!cancel_flag) { // Only show error if not cancelled
             std::cerr << _("A critical error occurred during processing. Please check the log/output.") << std::endl;
             return 1; // Exit with error code
        }
        // If cancelled, exit normally (log message already printed by core)
    }

    // Analysis completed successfully (or was cancelled gracefully)
    return 0;
}