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
#include "../utils/PathManager.hpp"
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

    // 6. Create the final gamma-corrected debug image.
    cv::Mat final_debug_image = CreateFinalDebugImage(patch_data.image_with_patches, patch_data.max_pixel_value);
    if (final_debug_image.empty() && !opts.print_patch_filename.empty()) {
        log_stream << "  - " << _("Warning: Could not generate debug patch image for this file. Input data may be invalid (e.g., bad keystone correction).") << std::endl;
    }

    return {dr_result, curve_data, final_debug_image};
}

} // end of anonymous namespace

ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    ProcessingResult result;
    // 1. Load files (I/O Responsibility)
    std::vector<RawFile> raw_files = LoadRawFiles(opts.input_files, log_stream);

    // 2. Attempt automatic corner detection if no manual coordinates are provided.
    std::optional<std::vector<cv::Point2d>> detected_corners_opt;
    if (opts.chart_coords.empty() && !raw_files.empty() && raw_files[0].IsLoaded()) {
        log_stream << _("Manual coordinates not provided, attempting iterative corner detection...") << std::endl;
        cv::Mat raw_img = raw_files[0].GetRawImage();
        cv::Mat img_float = NormalizeRawImage(raw_img, opts.dark_value, opts.saturation_value);

        int bayer_rows = img_float.rows / 2;
        int bayer_cols = img_float.cols / 2;
        cv::Mat mono_bayer = cv::Mat::zeros(bayer_rows, bayer_cols, CV_32FC1);
        for (int r = 0; r < bayer_rows; ++r) {
            for (int c = 0; c < bayer_cols; ++c) {
                float r_pix  = img_float.at<float>(r * 2,     c * 2);
                float g1_pix = img_float.at<float>(r * 2,     c * 2 + 1);
                float g2_pix = img_float.at<float>(r * 2 + 1, c * 2);
                float b_pix  = img_float.at<float>(r * 2 + 1, c * 2 + 1);
                mono_bayer.at<float>(r, c) = (r_pix + g1_pix + g2_pix + b_pix) / 4.0f;
            }
        }
        
        // Apply gamma correction to the monochrome image to enhance contrast for detection.
        cv::Mat gamma_corrected_bayer;
        cv::pow(mono_bayer, 1.0 / 2.2, gamma_corrected_bayer);

        // Detection loop with decreasing thresholds on the gamma-corrected image.
        for (int threshold_percent = 80; threshold_percent >= opts.min_corner_brightness; threshold_percent -= 10) {
            double threshold = static_cast<double>(threshold_percent) / 100.0;
            log_stream << "  - " << _("Attempting detection with brightness threshold > ") << threshold_percent << "%..." << std::endl;
            detected_corners_opt = DetectChartCorners(gamma_corrected_bayer, threshold, log_stream);
            if (detected_corners_opt.has_value()) {
                log_stream << "  - " << _("Detection successful at ") << threshold_percent << "% " << _("threshold.") << std::endl;
                break; // Exit loop on success.
            }
            if (threshold_percent > opts.min_corner_brightness) {
                 log_stream << "  - " << _("Detection failed, lowering threshold...") << std::endl;
            }
        }
    }

    // 3. Define the context for the analysis (e.g., which chart to use)
    ChartProfile chart(opts, detected_corners_opt, log_stream);
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    // 4. Orchestrate analysis for each file, respecting the keystone optimization setting.
    bool first_file_processed = false;
    if (DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION) {
        log_stream <<  _("Using optimized keystone: calculating parameters once for the series...") << std::endl;
        log_stream << _("Analyzing chart using a grid of ") << chart.GetGridCols() << _(" columns by ") << chart.GetGridRows() << _(" rows.") << std::endl;
        
        if (!opts.print_patch_filename.empty()) {
            PathManager paths(opts);
            fs::path debug_path = paths.GetCsvOutputPath().parent_path() / opts.print_patch_filename;
            log_stream << _("Debug patch image will be saved to: ") << debug_path.string() << std::endl;
        }

        Eigen::VectorXd keystone_params = CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
        for (const auto& raw_file : raw_files) {
            if (cancel_flag) return {};
            if (!raw_file.IsLoaded()) continue;
            auto file_result = AnalyzeSingleRawFile(raw_file, opts, chart, keystone_params, log_stream, opts.sensor_resolution_mpx);
            
            if (!first_file_processed && !file_result.final_debug_image.empty()) {
                result.debug_patch_image = file_result.final_debug_image;
                first_file_processed = true;
            }

            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    } else {
        log_stream << "Using non-optimized keystone: recalculating parameters for each image..." << std::endl;
        for (const auto& raw_file : raw_files) {
            if (cancel_flag) return {};
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