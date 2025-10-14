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
#include <filesystem>
#include <iostream>
#include <atomic>
#include <libintl.h>
#include <thread> 

#define _(string) gettext(string)

namespace { // Anonymous namespace for internal helper functions

std::vector<RawFile> LoadRawFiles(const std::vector<std::string>& input_files, std::ostream& log_stream) {
    std::vector<RawFile> raw_files;
    raw_files.reserve(input_files.size());
    for(const auto& filename : input_files) {
        raw_files.emplace_back(filename);
        if (!raw_files.back().Load()) {
            log_stream << _("Error: Could not load RAW file: ") << filename << std::endl;
        }
    }
    return raw_files;
}

} // end of anonymous namespace

ProcessingResult ProcessFiles(
    const AnalysisParameters& params,
    const PathManager& paths,
    std::ostream& log_stream,
    const std::atomic<bool>& cancel_flag,
    const std::vector<RawFile>& raw_files)
{
    // 1. Files are already loaded, so we proceed directly to analysis.
    // 2. Attempt automatic corner detection using only the selected source file.
    std::optional<std::vector<cv::Point2d>> detected_corners_opt;
    if (params.source_image_index < raw_files.size()) {
        detected_corners_opt = DynaRange::Engine::Processing::AttemptAutomaticCornerDetection(
            raw_files[params.source_image_index], // Pass the single RawFile object by const reference
            params.chart_coords, 
            params.dark_value, 
            params.saturation_value, 
            paths, 
            log_stream
        );
    }
    
    // 3. Define the analysis context
    ChartProfile chart(params.chart_coords, params.chart_patches_m, params.chart_patches_n, detected_corners_opt, log_stream);
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }
    
    log_stream << _("Analyzing chart using a grid of ") << chart.GetGridCols() << _(" columns by ") << chart.GetGridRows() << _(" rows.") << std::endl;
    if (!params.print_patch_filename.empty()) {
        fs::path debug_path = paths.GetCsvOutputPath().parent_path() / params.print_patch_filename;
        log_stream << _("Debug patch image will be saved to: ") << debug_path.string() << std::endl;
    }
    log_stream << _("Starting Dynamic Range calculation process...") << std::endl;
    // Add a log message indicating parallel processing will be used.
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 1; // Fallback for safety
    log_stream << _("Starting parallel processing with ") << num_threads << _(" threads...") << std::endl;
    
    // 4. Delegate the entire analysis loop to the specialized runner.
    // The missing 7th argument, params.source_image_index, is now provided.
    DynaRange::Engine::Processing::AnalysisLoopRunner runner(raw_files, params, chart, camera_model_name, log_stream, cancel_flag, params.source_image_index);
    
    return runner.Run();
}
