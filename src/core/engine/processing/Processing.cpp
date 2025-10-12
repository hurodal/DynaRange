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

ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    // 1. Load files
    std::vector<RawFile> raw_files = LoadRawFiles(opts.input_files, log_stream);
    PathManager paths(opts);

    // 2. Attempt automatic corner detection
    std::optional<std::vector<cv::Point2d>> detected_corners_opt = DynaRange::Engine::Processing::AttemptAutomaticCornerDetection(raw_files, opts, paths, log_stream);

    // 3. Define the analysis context
    ChartProfile chart(opts, detected_corners_opt, log_stream);
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }
    log_stream << _("Analyzing chart using a grid of ") << chart.GetGridCols() << _(" columns by ") << chart.GetGridRows() << _(" rows.") << std::endl;
    if (!opts.print_patch_filename.empty()) {
        fs::path debug_path = paths.GetCsvOutputPath().parent_path() / opts.print_patch_filename;
        log_stream << _("Debug patch image will be saved to: ") << debug_path.string() << std::endl;
    }
    log_stream << _("Starting Dynamic Range calculation process...") << std::endl;

    // 4. Delegate the entire analysis loop to the specialized runner.
    DynaRange::Engine::Processing::AnalysisLoopRunner runner(raw_files, opts, chart, camera_model_name, log_stream, cancel_flag);
    
    return runner.Run();
}