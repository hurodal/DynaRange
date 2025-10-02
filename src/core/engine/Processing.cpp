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
#include "../io/OutputWriter.hpp"
#include "../DebugConfig.hpp" 
#include "../Constants.hpp"
#include <filesystem>
#include <iostream>
#include <atomic>
#include <libintl.h>
#include <opencv2/imgproc.hpp>

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
 * @param generate_debug_image Only most low iso image must generate printpatches.png.
 * @return A SingleFileResult struct containing the analysis results.
 */
SingleFileResult AnalyzeSingleRawFile(
    const RawFile& raw_file, 
    const ProgramOptions& opts, 
    const ChartProfile& chart, 
    const Eigen::VectorXd& keystone_params,
    std::ostream& log_stream,
    double camera_resolution_mpx,
    bool generate_debug_image) // New parameter to control debug image generation
{
    log_stream << _("Processing \"") << fs::path(raw_file.GetFilename()).filename().string() << "\"..." << std::endl;
    
    // 1. Prepare the image for analysis.
    cv::Mat img_prepared = PrepareChartImage(raw_file, opts, keystone_params, chart, log_stream);
    if (img_prepared.empty()) {
        log_stream << _("Error: Failed to prepare image for analysis: ") << raw_file.GetFilename() << std::endl;
        return {};
    }
       
    // 2. Call Analysis module, passing the flag to conditionally create the overlay.
    PatchAnalysisResult patch_data = AnalyzePatches(img_prepared, chart.GetGridCols(), chart.GetGridRows(), opts.patch_ratio, generate_debug_image);
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

    // 6. Create the final gamma-corrected debug image only if it was generated.
    cv::Mat final_debug_image;
    if (generate_debug_image) {
        final_debug_image = CreateFinalDebugImage(patch_data.image_with_patches, patch_data.max_pixel_value);
        if (final_debug_image.empty()) {
            log_stream << "  - " << _("Warning: Could not generate debug patch image for this file. Input data may be invalid (e.g., bad keystone correction).") << std::endl;
        }
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
        log_stream << _("Manual coordinates not provided, attempting automatic corner detection...") << std::endl;
        cv::Mat raw_img = raw_files[0].GetRawImage();
        cv::Mat img_float = NormalizeRawImage(raw_img, opts.dark_value, opts.saturation_value);

        int bayer_rows = img_float.rows / 2;
        int bayer_cols = img_float.cols / 2;

        cv::Mat g1_bayer = cv::Mat::zeros(bayer_rows, bayer_cols, CV_32FC1);
        for (int r = 0; r < bayer_rows; ++r) {
            for (int c = 0; c < bayer_cols; ++c) {
                g1_bayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2 + 1);
            }
        }
        
        cv::threshold(g1_bayer, g1_bayer, 0.0, 0.0, cv::THRESH_TOZERO);
        detected_corners_opt = DetectChartCorners(g1_bayer, log_stream);

        #if DYNA_RANGE_DEBUG_MODE == 1
        if (DynaRange::Debug::ENABLE_CORNER_DETECTION_DEBUG && detected_corners_opt.has_value()) {
            log_stream << "  - [DEBUG] Saving corner detection visual confirmation to 'debug_corners_detected.png'..." << std::endl;
            cv::Mat viewable_image;
            cv::normalize(g1_bayer, viewable_image, 0.0, 1.0, cv::NORM_MINMAX);
            cv::Mat image_with_markers = DrawCornerMarkers(viewable_image, *detected_corners_opt);
            cv::Mat final_debug_image;
            cv::pow(image_with_markers, 1.0 / 2.2, final_debug_image);
            PathManager paths(opts);
            fs::path debug_path = paths.GetCsvOutputPath().parent_path() / "debug_corners_detected.png";
            OutputWriter::WriteDebugImage(final_debug_image, debug_path, log_stream);
        }
        #endif

        if (detected_corners_opt.has_value()) {
            std::vector<cv::Point2f> corners_float;
            for (const auto& pt : *detected_corners_opt) {
                corners_float.push_back(cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y)));
            }
            double total_image_area = static_cast<double>(g1_bayer.cols * g1_bayer.rows);
            double detected_chart_area = cv::contourArea(corners_float);
            double area_percentage = (detected_chart_area / total_image_area);
            if (area_percentage < DynaRange::Constants::MINIMUM_CHART_AREA_PERCENTAGE) {
                log_stream << _("Warning: Automatic corner detection found an area covering only ")
                           << std::fixed << std::setprecision(1) << (area_percentage * 100.0)
                           << _("% of the image. This is below the required threshold of ")
                           << (DynaRange::Constants::MINIMUM_CHART_AREA_PERCENTAGE * 100.0)
                           << _(" %. Discarding detected corners and falling back to defaults.") << std::endl;
                detected_corners_opt.reset();
            }
        }
    }

    // 3. Define the context for the analysis (e.g., which chart to use)
    ChartProfile chart(opts, detected_corners_opt, log_stream);
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    // 4. Orchestrate analysis for each file.
    if (DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION) {
        log_stream <<  _("Using optimized keystone: calculating parameters once for the series...") << std::endl;
        log_stream << _("Analyzing chart using a grid of ") << chart.GetGridCols() << _(" columns by ") << chart.GetGridRows() << _(" rows.") << std::endl;
        if (!opts.print_patch_filename.empty()) {
            PathManager paths(opts);
            fs::path debug_path = paths.GetCsvOutputPath().parent_path() / opts.print_patch_filename;
            log_stream << _("Debug patch image will be saved to: ") << debug_path.string() << std::endl;
        }
        log_stream << _("Starting Dynamic Range calculation process...") << std::endl;
        Eigen::VectorXd keystone_params = CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());

        for (size_t i = 0; i < raw_files.size(); ++i) {
            if (cancel_flag) return {};
            const auto& raw_file = raw_files[i];
            if (!raw_file.IsLoaded()) continue;

            // The flag is true only for the first file and only if requested by user.
            bool generate_debug_image = (i == 0 && !opts.print_patch_filename.empty());

            auto file_result = AnalyzeSingleRawFile(raw_file, opts, chart, keystone_params, log_stream, opts.sensor_resolution_mpx, generate_debug_image);
            
            // If a debug image was generated (only happens once), store it.
            if (!file_result.final_debug_image.empty()) {
                result.debug_patch_image = file_result.final_debug_image;
            }

            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    } else {
        // This non-optimized path is less common and kept simple.
        // It will still only save the first debug image due to the logic in FinalizeAndReport.
        log_stream << "Using non-optimized keystone: recalculating parameters for each image..." << std::endl;
        for (const auto& raw_file : raw_files) {
            if (cancel_flag) return {};
            if (!raw_file.IsLoaded()) continue;
            Eigen::VectorXd keystone_params = CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
            // For simplicity, we pass 'true' to always generate, but only the first is kept later.
            auto file_result = AnalyzeSingleRawFile(raw_file, opts, chart, keystone_params, log_stream, opts.sensor_resolution_mpx, !opts.print_patch_filename.empty());
            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    }
    return result;
}