// File: src/core/engine/Processing.cpp
/**
 * @file core/engine/Processing.cpp
 * @brief Implements the core logic for processing and analyzing RAW files.
 */
#include "Processing.hpp"
#include "../io/RawFile.hpp"
#include "../graphics/ImageProcessing.hpp"
#include "../setup/ChartProfile.hpp"
#include "../analysis/ImageAnalyzer.hpp"
#include <filesystem>
#include <iostream>
#include <atomic>
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

// Anonymous namespace for internal helper functions.
namespace {
/**
 * @brief Loads a list of RAW file paths into RawFile objects.
 * @param input_files Vector of file paths.
 * @param log_stream Stream for logging messages.
 * @return A vector of loaded RawFile objects.
 */
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
/**
 * @brief (Orchestrator) Analyzes a single RAW file by calling the appropriate modules.
 * @param raw_file The RawFile object to be analyzed.
 * @param opts The program options.
 * @param chart The chart profile defining the geometry.
 * @param keystone_params The pre-calculated keystone transformation parameters.
 * @param log_stream The output stream for logging.
 * @param camera_resolution_mpx The resolution of the camera sensor in megapixels.
 * @return A SingleFileResult struct containing the analysis results.
 */
SingleFileResult AnalyzeSingleRawFile(
    const RawFile& raw_file, 
    const ProgramOptions& opts, 
    const ChartProfile& chart, 
    const Eigen::VectorXd& keystone_params,
    std::ostream& log_stream,
    double camera_resolution_mpx)
{
    log_stream << _("Processing \"") << fs::path(raw_file.GetFilename()).filename().string() << "\"..." << std::endl;
    
    // All image preparation logic is now delegated to the ImageProcessing module,
    // making this function a pure orchestrator, thus adhering to SRP.
    
    // 1. Prepare the image for analysis.
    cv::Mat img_prepared = PrepareChartImage(raw_file, opts, keystone_params, chart, log_stream);
    if (img_prepared.empty()) {
        log_stream << _("Error: Failed to prepare image for analysis: ") << raw_file.GetFilename() << std::endl;
        return {};
    }
    
    // 2. Call Analysis module to find patches on the prepared image.
    PatchAnalysisResult patch_data = AnalyzePatches(img_prepared, chart.GetGridCols(), chart.GetGridRows(), opts.patch_ratio);
    if (patch_data.signal.empty()) {
        log_stream << _("Warning: No valid patches found for ") << raw_file.GetFilename() << std::endl;
        return {};
    }
    
    // 3. Call Analysis module to perform calculations.
    auto [dr_result, curve_data] = CalculateResultsFromPatches(patch_data, opts, raw_file.GetFilename(), camera_resolution_mpx);
    
    // 4. Assign the correct plot label from the map populated in the setup phase.
    if(opts.plot_labels.count(raw_file.GetFilename())) {
        curve_data.plot_label = opts.plot_labels.at(raw_file.GetFilename());
    } else {
        curve_data.plot_label = fs::path(raw_file.GetFilename()).stem().string();
    }
    
    // 5. Store the numeric ISO speed for the individual plot title.
    curve_data.iso_speed = raw_file.GetIsoSpeed();
    return {dr_result, curve_data};
}

} // end of anonymous namespace

ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    ProcessingResult result;

    // 1. Load files (I/O Responsibility)
    std::vector<RawFile> raw_files = LoadRawFiles(opts.input_files, log_stream);

    // 2. Define the context for the analysis (e.g., which chart to use)
    // The ChartProfile is now constructed with the program options,
    // allowing it to use custom coordinates if they were provided.
    ChartProfile chart(opts);
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    // 3. Orchestrate analysis for each file, respecting the keystone optimization setting.
    if (DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION) {
        log_stream <<  _("Using optimized keystone: calculating parameters once for the series...") << std::endl;
        // Add a log message to inform the user about the grid dimensions being used.
        log_stream << _("Analyzing chart using a grid of ") << chart.GetGridCols() << _(" columns by ") << chart.GetGridRows() << _(" rows.") << "" << std::endl;
        Eigen::VectorXd keystone_params = CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
        for (const auto& raw_file : raw_files) {
            if (cancel_flag) return {}; // Cancellation check
            if (!raw_file.IsLoaded()) continue;
            auto file_result = AnalyzeSingleRawFile(raw_file, opts, chart, keystone_params, log_stream, opts.sensor_resolution_mpx);
            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    } else {
        log_stream << "Using non-optimized keystone: recalculating parameters for each image..." << std::endl;
        for (const auto& raw_file : raw_files) {
            if (cancel_flag) return {}; // Cancellation check
            if (!raw_file.IsLoaded()) continue;
            Eigen::VectorXd keystone_params = CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
            auto file_result = AnalyzeSingleRawFile(raw_file, opts, chart, keystone_params, log_stream, opts.sensor_resolution_mpx);
            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    }
    return result;
}
