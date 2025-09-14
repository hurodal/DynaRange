/**
 * @file core/engine/Processing.cpp
 * @brief Implements the core logic for processing and analyzing RAW files.
 */
#include "Processing.hpp"
#include "../RawFile.hpp" 
#include "../ImageProcessing.hpp"
#include <filesystem>
#include <iostream>

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
            log_stream << "Error: Could not load RAW file: " << filename << std::endl;
        }
    }
    return raw_files;
}

/**
 * @brief Prepares an image for analysis by normalizing, correcting, and cropping it.
 * @param raw_file The source RawFile object.
 * @param opts The program options.
 * @param k The keystone correction parameters.
 * @param log_stream Stream for logging messages.
 * @return The prepared cv::Mat image, ready for patch analysis.
 */
cv::Mat PrepareImageForAnalysis(const RawFile& raw_file, const ProgramOptions& opts, const Eigen::VectorXd& k, std::ostream& log_stream) {
    cv::Mat img_float = raw_file.GetNormalizedImage(opts.dark_value, opts.saturation_value);
    if(img_float.empty()){
        log_stream << "Error: Could not get normalized image for: " << raw_file.GetFilename() << std::endl;
        return {};
    }
    log_stream << "  - Info: Black=" << opts.dark_value << ", Saturation=" << opts.saturation_value << std::endl;

    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    for (int r = 0; r < imgBayer.rows; ++r) {
        for (int c = 0; c < imgBayer.cols; ++c) {
            imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
        }
    }
    
    cv::Mat imgc = UndoKeystone(imgBayer, k);
    double xtl = (119.0 + 99.0) / 2.0; double ytl = (170.0 + 158.0) / 2.0;
    double xbr = (2515.0 + 2473.0) / 2.0; double ybr = (1687.0 + 1679.0) / 2.0;
    cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
    return imgc(crop_area);
}

/**
 * @brief (Orchestrator) Analyzes a single RAW file by calling the appropriate modules.
 */
SingleFileResult AnalyzeSingleRawFile(const RawFile& raw_file, const ProgramOptions& opts, const ChartProfile& chart, std::ostream& log_stream) {
    log_stream << "\nProcessing \"" << fs::path(raw_file.GetFilename()).filename().string() << "\"..." << std::endl;

    // 1. Call ImageProcessing module to prepare the image
    cv::Mat img_prepared = PrepareChartImage(raw_file, opts, chart, log_stream);
    if (img_prepared.empty()) {
        return {};
    }

    // 2. Call Analysis module to find patches
    PatchAnalysisResult patch_data = AnalyzePatches(img_prepared.clone(), chart.GetGridCols(), chart.GetGridRows(), opts.patch_ratio);
    if (patch_data.signal.empty()) {
        log_stream << "Warning: No valid patches found for " << raw_file.GetFilename() << std::endl;
        return {};
    }

    // 3. Call Analysis module to perform calculations
    auto [dr_result, curve_data] = CalculateResultsFromPatches(patch_data, opts, raw_file.GetFilename());

    return {dr_result, curve_data};
}

} // end of anonymous namespace


ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream) {
    ProcessingResult result;
    
    // 1. Load files (I/O Responsibility)
    std::vector<RawFile> raw_files = LoadRawFiles(opts.input_files, log_stream);
    
    // 2. Define the context for the analysis (e.g., which chart to use)
    ChartProfile chart; // The analysis will use the default chart profile.
    
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    // 3. Orchestrate analysis for each file
    for (const auto& raw_file : raw_files) {
        if (!raw_file.IsLoaded()) continue;

        auto file_result = AnalyzeSingleRawFile(raw_file, opts, chart, log_stream);
        
        // Aggregate valid results
        if (!file_result.dr_result.filename.empty()) {
            file_result.curve_data.camera_model = camera_model_name;
            result.dr_results.push_back(file_result.dr_result);
            result.curve_data.push_back(file_result.curve_data);
        }
    }
    return result;
}