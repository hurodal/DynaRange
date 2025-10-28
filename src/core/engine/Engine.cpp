// File: src/core/engine/Engine.cpp
/**
 * @file src/core/Engine.cpp
 * @brief Implements the main orchestrator for the analysis workflow.
 */
#include "Engine.hpp"
#include "Initialization.hpp"
#include "processing/Processing.hpp"
#include "Reporting.hpp"
#include "Validation.hpp"
#include "../artifacts/image/DebugImageWriter.hpp"
#include "../arguments/ArgumentsOptions.hpp"
#include "../utils/OutputNamingContext.hpp"
#include "../utils/PathManager.hpp"
#include <atomic>
#include <ostream>
#include <string>       
#include <vector> 
#include <libintl.h>
#include <filesystem>  

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace DynaRange {

/**
 * @brief Orchestrates the entire dynamic range analysis workflow from start to finish.
 * @details Manages the four phases: Initialization, Processing, Validation, Reporting.
 * Uses ArtifactFactory for generating debug output images.
 * @param opts A reference to the program options, used throughout the process.
 * @param log_stream The output stream for logging all messages.
 * @param cancel_flag An atomic boolean flag for requesting cancellation.
 * @return A ReportOutput struct containing paths to generated files and final results,
 * or an empty struct on failure or cancellation.
 */
ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    // Phase 1: Preparation
    InitializationResult init_result = InitializeAnalysis(opts, log_stream);
    if (!init_result.success) {
        log_stream << _("Error during initialization phase. Aborting.") << std::endl;
        return {}; // Return empty report on initialization failure
    }

    // Create PathManager based on potentially updated options (e.g., output_filename)
    PathManager paths(opts);
    // Determine camera model from loaded files (use first file as representative)
    std::string camera_model = "UNKNOWN_CAMERA";
    if (!init_result.loaded_raw_files.empty() && init_result.loaded_raw_files[0].IsLoaded()) {
        camera_model = init_result.loaded_raw_files[0].GetCameraModel();
        // Update opts if resolution wasn't detected earlier (though InitializeAnalysis should do this)
        if (opts.sensor_resolution_mpx <= 0.0) {
             opts.sensor_resolution_mpx = init_result.sensor_resolution_mpx;
        }
    }

    // Populate the AnalysisParameters struct needed by the processing phase
    // Copy relevant values from InitializationResult and ProgramOptions
    AnalysisParameters analysis_params {
        .dark_value = init_result.dark_value,
        .saturation_value = init_result.saturation_value,
        .poly_order = opts.poly_order,
        .dr_normalization_mpx = opts.dr_normalization_mpx,
        .snr_thresholds_db = opts.snr_thresholds_db,
        .patch_ratio = opts.patch_ratio,
        .sensor_resolution_mpx = opts.sensor_resolution_mpx, // Use final value from opts/init_result
        .chart_coords = opts.chart_coords,
        .chart_patches_m = opts.GetChartPatchesM(),
        .chart_patches_n = opts.GetChartPatchesN(),
        .raw_channels = opts.raw_channels,
        .print_patch_filename = opts.print_patch_filename, // Pass user request/sentinel
        .plot_labels = init_result.plot_labels,
        .generated_command = init_result.generated_command,
        .source_image_index = init_result.source_image_index,
        .generate_full_debug = opts.generate_full_debug // Copiar flag desde ProgramOptions
    };

    // Phase 2: Processing - Run the analysis loop over the loaded RAW files
    ProcessingResult results = ProcessFiles(analysis_params, paths, log_stream, cancel_flag, init_result.loaded_raw_files);
    // Guardar PrintPatches DESPUÉS del procesamiento usando Factory
    if (results.debug_patch_image.has_value() && !analysis_params.print_patch_filename.empty())
    {
        // Create context needed by the Factory
        OutputNamingContext naming_ctx_patches;
        naming_ctx_patches.camera_name_exif = camera_model; // Use detected model

        // Determine effective camera name based on opts
        std::string effective_name = "";
        if (opts.gui_use_camera_suffix) {
            if (opts.gui_use_exif_camera_name) {
                effective_name = camera_model; // Use EXIF name
            } else {
                effective_name = opts.gui_manual_camera_name; // Use Manual name
            }
        }
        naming_ctx_patches.effective_camera_name_for_output = effective_name;
        naming_ctx_patches.user_print_patches_filename = analysis_params.print_patch_filename; // Pass user request/sentinel

        // Call Factory to create the file
        // The factory handles filename generation and writing
        std::optional<fs::path> saved_path = ArtifactFactory::Image::CreatePrintPatchesImage(
            *results.debug_patch_image,
            naming_ctx_patches,
            paths,
            log_stream);
        // We don't necessarily need the returned path here, but logging is inside the Factory.
        if (!saved_path) {
             log_stream << _("Warning: Failed to save debug patches image.") << std::endl;
        }
    }

    // Check for cancellation after processing phase
    if (cancel_flag) {
        log_stream << "\n" << _("[INFO] Analysis cancelled by user during processing.") << std::endl;
        return {}; // Return empty report
    }
    // Check if processing yielded any results before validation/reporting
    if (results.dr_results.empty() && results.curve_data.empty()) {
        log_stream << _("\nError: Processing phase did not yield any valid results.") << std::endl;
        return {};
    }


    // Phase 3: Validation - Check sufficiency of SNR data for requested thresholds
    ValidateSnrResults(results, analysis_params, log_stream);
    // Phase 4: Reporting - Generate CSV and plot files
    // Populate ReportingParameters struct needed by the reporting phase
    ReportingParameters reporting_params {
        .raw_channels = opts.raw_channels,
        .generate_plot = opts.generate_plot,
        .generate_individual_plots = opts.generate_individual_plots,
        .plot_format = opts.plot_format,
        .plot_details = opts.plot_details,
        .plot_command_mode = opts.plot_command_mode,
        .generated_command = init_result.generated_command, // Command string for plot footer
        .dark_value = init_result.dark_value,
        .saturation_value = init_result.saturation_value,
        .black_level_is_default = init_result.black_level_is_default,
        .saturation_level_is_default = init_result.saturation_level_is_default,
        .snr_thresholds_db = opts.snr_thresholds_db,
        // Copiar los flags de la GUI/CLI desde opts a reporting_params
        .gui_manual_camera_name = opts.gui_manual_camera_name,
        .gui_use_exif_camera_name = opts.gui_use_exif_camera_name,
        .gui_use_camera_suffix = opts.gui_use_camera_suffix
    };

    // Call the already modified FinalizeAndReport
    ReportOutput report = FinalizeAndReport(results, reporting_params, paths, log_stream);
    // Add final results data to the report struct (for GUI presenter)
    report.dr_results = results.dr_results;
    report.curve_data = results.curve_data;

    // Return the final report containing paths and results
    return report;
}
} // namespace DynaRange