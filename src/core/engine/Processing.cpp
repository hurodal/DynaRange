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
#include "../graphics/ImageProcessing.hpp"
#include "../io/OutputWriter.hpp"
#include <filesystem>
#include <iostream>
#include <atomic>
#include <libintl.h>
#include <opencv2/imgcodecs.hpp>
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

    // 1. Prepare the single-channel bayer image for analysis.
    cv::Mat img_prepared = PrepareChartImage(raw_file, opts, keystone_params, chart, log_stream);
    if (img_prepared.empty()) {
        log_stream << _("Error: Failed to prepare image for analysis: ") << raw_file.GetFilename() << std::endl;
        return {};
    }

    double max_pixel_value;
    cv::minMaxLoc(img_prepared, nullptr, &max_pixel_value);

    // 2. Analyze patches on the prepared image. This function will draw rectangles on it.
    PatchAnalysisResult patch_data = AnalyzePatches(img_prepared, chart.GetGridCols(), chart.GetGridRows(), opts.patch_ratio);
    if (patch_data.signal.empty()) {
        log_stream << _("Warning: No valid patches found for ") << raw_file.GetFilename() << std::endl;
        return {};
    }

    // 3. Perform calculations.
    auto [dr_result, curve_data] = CalculateResultsFromPatches(patch_data, opts, raw_file.GetFilename(), camera_resolution_mpx);
    
    if(opts.plot_labels.count(raw_file.GetFilename())) {
        curve_data.plot_label = opts.plot_labels.at(raw_file.GetFilename());
    } else {
        curve_data.plot_label = fs::path(raw_file.GetFilename()).stem().string();
    }
    
    curve_data.iso_speed = raw_file.GetIsoSpeed();

    // 4. Generate the final gamma-corrected debug image from the single-channel overlay.
    cv::Mat final_debug_image = CreateFinalDebugImage(patch_data.image_with_patches, max_pixel_value);

    return {dr_result, curve_data, final_debug_image};
}

} // end of anonymous namespace

ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    ProcessingResult result;
    std::vector<RawFile> raw_files = LoadRawFiles(opts.input_files, log_stream);
    
    ProgramOptions local_opts = opts;

    if (local_opts.chart_coords.empty() && !raw_files.empty() && raw_files[0].IsLoaded()) {
        log_stream << _("Chart coordinates not provided. Attempting automatic detection...") << std::endl;
        
        cv::Mat raw_img = raw_files[0].GetRawImage();
        cv::Mat bayer_img(raw_img.rows / 2, raw_img.cols / 2, CV_32FC1);
        cv::Mat float_img = NormalizeRawImage(raw_img, local_opts.dark_value, local_opts.saturation_value);
        for (int r = 0; r < bayer_img.rows; ++r) {
            for (int c = 0; c < bayer_img.cols; ++c) {
                 bayer_img.at<float>(r, c) = float_img.at<float>(r * 2, c * 2 + 1);
            }
        }
        
        auto detected_corners_opt = DetectChartCorners(bayer_img, log_stream);
        if (detected_corners_opt) {
            log_stream << _("Automatic corner detection successful.") << std::endl;
            const auto& points = *detected_corners_opt;
            auto format_point_str = [](const cv::Point2d& p) {
                std::stringstream ss;
                ss << "[" << std::right << std::setw(5) << static_cast<int>(round(p.x * 2.0))
                   << ", " << std::right << std::setw(5) << static_cast<int>(round(p.y * 2.0)) << " ]";
                return ss.str();
            };
            std::string tl_str = "  TL-> " + format_point_str(points[0]);
            std::string tr_str = format_point_str(points[3]) + " <-TR";
            std::string bl_str = "  BL-> " + format_point_str(points[1]);
            std::string br_str = format_point_str(points[2]) + " <-BR";
            log_stream << _("  - Detected Coords:") << std::endl;
            log_stream << tl_str << "\t" << tr_str << std::endl;
            log_stream << bl_str << "\t" << br_str << std::endl;
            std::vector<double> full_size_coords;
            for (const auto& point : *detected_corners_opt) {
                full_size_coords.push_back(point.x * 2.0);
                full_size_coords.push_back(point.y * 2.0);
            }
            local_opts.chart_coords = full_size_coords;
        } else {
            log_stream << _("Warning: Automatic corner detection failed. Falling back to default coordinates.") << std::endl;
        }
    }
    
    ChartProfile chart(local_opts);
    
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    if (DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION) {
        Eigen::VectorXd keystone_params = CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
        
        for (size_t i = 0; i < raw_files.size(); ++i) {
            if (cancel_flag) return {};
            if (!raw_files[i].IsLoaded()) continue;

            auto file_result = AnalyzeSingleRawFile(raw_files[i], local_opts, chart, keystone_params, log_stream, local_opts.sensor_resolution_mpx);
            
            // This block now generates a full-color, human-readable debug image.
            if (i == 0 && local_opts.print_patch_mode && !file_result.final_debug_image.empty()) {
                PathManager paths(local_opts);
                fs::path debug_image_path = paths.GetCsvOutputPath().parent_path() / "chartpatches.png";
                
                // 1. Get the full-color, demosaiced image to use as a background.
                cv::Mat background_image = raw_files[i].GetProcessedImage();

                if (!background_image.empty()) {
                    // 2. Apply the same keystone correction and crop to the color image.
                    cv::Mat corrected_bg = UndoKeystoneColor(background_image, keystone_params);
                    const auto& dst_pts = chart.GetDestinationPoints();
                    // The coordinates for cropping are scaled by 0.5 because the keystone params
                    // were calculated on a half-resolution bayer image.
                    cv::Rect crop_area(round(dst_pts[0].x * 0.5), round(dst_pts[0].y * 0.5),
                                       round((dst_pts[2].x - dst_pts[0].x) * 0.5), round((dst_pts[2].y - dst_pts[0].y) * 0.5));
                    
                    if (crop_area.x >= 0 && crop_area.y >= 0 && crop_area.width > 0 && crop_area.height > 0 &&
                        crop_area.x + crop_area.width <= corrected_bg.cols &&
                        crop_area.y + crop_area.height <= corrected_bg.rows) {

                        cv::Mat cropped_bg = corrected_bg(crop_area);

                        // 3. Create a color version of the patch overlay
                        cv::Mat overlay_8u, overlay_color, mask;
                        file_result.final_debug_image.convertTo(overlay_8u, CV_8U, 255.0);
                        cv::cvtColor(overlay_8u, overlay_color, cv::COLOR_GRAY2BGR);
                        
                        // 4. Combine the color background with the patch overlay using alpha blending
                        cv::addWeighted(cropped_bg, 0.5, overlay_color, 0.5, 0.0, cropped_bg);
                        
                        if (OutputWriter::WriteDebugImage(cropped_bg, debug_image_path, log_stream)) {
                            // Success is logged inside the writer
                        }
                    }
                }
            }

            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    } else {
        log_stream << "Using non-optimized keystone: recalculating parameters for each image..." << std::endl;
        for (size_t i = 0; i < raw_files.size(); ++i) {
            if (cancel_flag) return {};
            if (!raw_files[i].IsLoaded()) continue;
            
            Eigen::VectorXd keystone_params = CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
            auto file_result = AnalyzeSingleRawFile(raw_files[i], local_opts, chart, keystone_params, log_stream, local_opts.sensor_resolution_mpx);

            if (i == 0 && local_opts.print_patch_mode && !file_result.final_debug_image.empty()) {
                PathManager paths(local_opts);
                fs::path debug_image_path = paths.GetCsvOutputPath().parent_path() / "chartpatches.png";
                cv::Mat background_image = raw_files[i].GetProcessedImage();
                if (!background_image.empty()) {
                    cv::Mat corrected_bg = UndoKeystoneColor(background_image, keystone_params);
                    const auto& dst_pts = chart.GetDestinationPoints();
                    cv::Rect crop_area(round(dst_pts[0].x * 0.5), round(dst_pts[0].y * 0.5),
                                       round((dst_pts[2].x - dst_pts[0].x) * 0.5), round((dst_pts[2].y - dst_pts[0].y) * 0.5));
                    if (crop_area.x >= 0 && crop_area.y >= 0 && crop_area.width > 0 && crop_area.height > 0 &&
                        crop_area.x + crop_area.width <= corrected_bg.cols &&
                        crop_area.y + crop_area.height <= corrected_bg.rows) {
                            
                        cv::Mat cropped_bg = corrected_bg(crop_area);
                        cv::Mat overlay_8u, overlay_color, mask;
                        file_result.final_debug_image.convertTo(overlay_8u, CV_8U, 255.0);
                        cv::cvtColor(overlay_8u, overlay_color, cv::COLOR_GRAY2BGR);
                        cv::addWeighted(cropped_bg, 0.5, overlay_color, 0.5, 0.0, cropped_bg);
                        if (OutputWriter::WriteDebugImage(cropped_bg, debug_image_path, log_stream)) {
                            // Success is logged inside the writer
                        }
                    }
                }
            }

            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    }

    return result;
}