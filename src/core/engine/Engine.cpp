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
#include <atomic>
#include <ostream>
#include <libintl.h>

#define _(string) gettext(string)

namespace DynaRange {

/**
 * @brief Orchestrates the entire dynamic range analysis workflow from start to finish.
 * @param opts A reference to the program options, which may be updated during initialization.
 * @param log_stream The output stream for logging all messages.
 * @param cancel_flag An atomic boolean flag that can be set from another thread to request cancellation.
 * @return A ReportOutput struct containing paths to the generated plots, or an empty struct on failure or cancellation.
 */
ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    // Phase 1: Preparation
    InitializationResult init_result = InitializeAnalysis(opts, log_stream);
    if (!init_result.success) {
        return {}; // Return an empty ReportOutput on failure
    }

    // --- New Logic: Select the source image for corner/patch detection ---
    int source_image_index = 0; // Default to the darkest file (index 0)
    bool found_non_saturated = false;
    // Iterate backwards from the brightest file to find the first non-saturated one.
    for (int i = init_result.loaded_raw_files.size() - 1; i >= 0; --i) {
        const auto& raw_file = init_result.loaded_raw_files[i];
        if (raw_file.IsLoaded()) {
            cv::Mat active_img = raw_file.GetActiveRawImage();
            if (!active_img.empty()) {
                // A pixel is considered saturated if it's at 99% or more of the saturation level.
                int saturated_pixels = cv::countNonZero(active_img >= (init_result.saturation_value * 0.99));
                if (saturated_pixels == 0) {
                    source_image_index = i;
                    found_non_saturated = true;
                    break;
                }
            }
        }
    }

    if (found_non_saturated) {
        log_stream << _("[INFO] Selected '") << fs::path(init_result.loaded_raw_files[source_image_index].GetFilename()).filename().string()
                   << _("' as the source image for detection (brightest non-saturated).") << std::endl;
    } else {
        log_stream << _("[INFO] All images contain saturated pixels. Selected '") << fs::path(init_result.loaded_raw_files[source_image_index].GetFilename()).filename().string()
                   << _("' as the source image for detection (darkest).") << std::endl;
    }
    // --- End New Logic ---
    
    PathManager paths(opts);
    // Create the analysis parameters struct from the explicit results of the initialization phase.
    AnalysisParameters analysis_params {
        .dark_value = init_result.dark_value,
        .saturation_value = init_result.saturation_value,
        .poly_order = opts.poly_order,
        .dr_normalization_mpx = opts.dr_normalization_mpx,
        .snr_thresholds_db = opts.snr_thresholds_db,
        .patch_ratio = opts.patch_ratio,
        .sensor_resolution_mpx = init_result.sensor_resolution_mpx,
        .chart_coords = opts.chart_coords,
        .chart_patches_m = opts.GetChartPatchesM(),
        
        .chart_patches_n = opts.GetChartPatchesN(),
        .raw_channels = opts.raw_channels,
        .print_patch_filename = opts.print_patch_filename,
        .plot_labels = init_result.plot_labels,
        .generated_command = init_result.generated_command,
        .source_image_index = source_image_index // Pass the selected index
    };

    // Phase 2: Processing of all files, now using the decoupled parameters.
    ProcessingResult results = ProcessFiles(analysis_params, paths, log_stream, cancel_flag, init_result.loaded_raw_files);
    if (cancel_flag) {
        log_stream << "\n" << _("[INFO] Analysis cancelled by user.") << std::endl;
        return {};
    }

    // Phase 3: Validate SNR data before final reporting
    ValidateSnrResults(results, analysis_params, log_stream);

    // Create the reporting parameters struct to decouple the reporting phase.
    ReportingParameters reporting_params {
        .raw_channels = opts.raw_channels,
        .generate_plot = opts.generate_plot,
        .generate_individual_plots = opts.generate_individual_plots,
        .plot_format = opts.plot_format,
        .plot_details = opts.plot_details,
        .plot_command_mode = opts.plot_command_mode,
        .generated_command = init_result.generated_command,
        .dark_value = init_result.dark_value,
        .saturation_value = init_result.saturation_value,
        .black_level_is_default = init_result.black_level_is_default,
        
        .saturation_level_is_default = init_result.saturation_level_is_default,
        .snr_thresholds_db = opts.snr_thresholds_db
    };

    // Phase 4: Generation of final reports, now using the decoupled parameters.
    ReportOutput report = FinalizeAndReport(results, reporting_params, paths, log_stream);
    report.dr_results = results.dr_results;
    report.curve_data = results.curve_data;

    return report;
}
} // namespace DynaRange