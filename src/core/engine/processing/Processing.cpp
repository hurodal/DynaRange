// File: src/core/engine/processing/Processing.cpp
/**
 * @file core/engine/processing/Processing.cpp
 * @brief Implements the high-level orchestration of the file processing workflow.
 */
#include "Processing.hpp"
#include "CornerDetectionHandler.hpp"
#include "AnalysisLoopRunner.hpp"
#include "../../io/raw/RawFile.hpp"
#include "../../setup/ChartProfile.hpp"
#include "../../utils/PathManager.hpp"
#include <iostream>
#include <atomic>
#include <libintl.h>
#include <thread> 

#define _(string) gettext(string)

ProcessingResult ProcessFiles(
    const AnalysisParameters& params,
    const PathManager& paths,
    std::ostream& log_stream,
    const std::atomic<bool>& cancel_flag,
    const std::vector<RawFile>& raw_files)
{
    // 1. Files are already loaded.

    // 2. Attempt automatic corner detection using the selected source file.
    std::optional<std::vector<cv::Point2d>> detected_corners_opt;
    // Check if the source index is valid before accessing raw_files
    if (params.source_image_index >= 0 && static_cast<size_t>(params.source_image_index) < raw_files.size()) {
        detected_corners_opt = DynaRange::Engine::Processing::AttemptAutomaticCornerDetection(
            raw_files[params.source_image_index], // Pass the single RawFile object
            params.chart_coords,
            params.dark_value,
            params.saturation_value,
            paths, // Pass PathManager for debug image path generation inside
            log_stream
        );
    } else if (!raw_files.empty()) {
        // Log a warning if the index is invalid but files exist? Could default to 0?
        log_stream << _("Warning: Invalid source_image_index provided. Skipping automatic corner detection.") << std::endl;
        // Proceed without automatic detection (will use manual or defaults)
    }
    // Note: If raw_files is empty, detected_corners_opt remains nullopt.

    // Make a local copy of params if needed? Currently not modifying params, so const ref is fine.
    // AnalysisParameters local_params = params; // Not needed currently

    // 3. Define the chart profile using manual, detected, or default corners.
    ChartProfile chart(params.chart_coords, params.chart_patches_m, params.chart_patches_n, detected_corners_opt, log_stream);

    // Get camera model name from the first loaded file (if any).
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    log_stream << _("Analyzing chart using a grid of ") << chart.GetGridCols() << _(" columns by ") << chart.GetGridRows() << _(" rows.") << std::endl;
    log_stream << _("Starting Dynamic Range calculation process...") << std::endl;

    // Determine number of threads for parallel processing.
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 1; // Fallback to at least one thread
    }
    log_stream << _("Starting parallel processing with ") << num_threads << _(" threads...") << std::endl;

    // 4. Delegate the entire analysis loop over all files to the specialized runner.
    // Pass const reference to params as it's not modified here.
    DynaRange::Engine::Processing::AnalysisLoopRunner runner(raw_files, params, chart, camera_model_name, log_stream, cancel_flag, params.source_image_index, paths);
    return runner.Run(); // Execute the parallel loop and return results.
}