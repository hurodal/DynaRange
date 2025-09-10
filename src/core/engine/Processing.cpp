// File: core/engine/Processing.cpp
#include "Processing.hpp"
#include "../graphics/Plotting.hpp"
#include "../RawFile.hpp" 
#include <Eigen/Dense>
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

// Internal helper functions for this file
namespace {

// This function now calculates DR for all requested thresholds and populates the results map.
SingleFileResult AnalyzeSingleRawFile(const RawFile& raw_file, const ProgramOptions& opts, const Eigen::VectorXd& k, std::ostream& log_stream) {
    log_stream << "\nProcessing \"" << fs::path(raw_file.GetFilename()).filename().string() << "\"..." << std::endl;

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
    cv::Mat imgcrop = imgc(crop_area);
    PatchAnalysisResult patch_data = AnalyzePatches(imgcrop.clone(), 11, 7, opts.patch_ratio);
    
    if (patch_data.signal.empty()) {
        log_stream << "Warning: No valid patches found for " << raw_file.GetFilename() << std::endl;
        return {};
    }

    std::vector<double> snr_db, signal_ev;
    for (size_t j = 0; j < patch_data.signal.size(); ++j) {
        snr_db.push_back(20 * log10(patch_data.signal[j] / patch_data.noise[j]));
        signal_ev.push_back(log2(patch_data.signal[j]));
    }

    cv::Mat signal_mat_global(signal_ev.size(), 1, CV_64F, signal_ev.data());
    cv::Mat snr_mat_global(snr_db.size(), 1, CV_64F, snr_db.data());
    
    cv::Mat poly_coeffs;
    PolyFit(signal_mat_global, snr_mat_global, poly_coeffs, opts.poly_order);
    
    auto min_max_ev = std::minmax_element(signal_ev.begin(), signal_ev.end());

    // Flexible DR calculation ---
    DynamicRangeResult dr_result;
    dr_result.filename = raw_file.GetFilename();
    dr_result.patches_used = (int)patch_data.signal.size();

    // Calculate DR for each threshold requested in the program options.
    for (const double threshold_db : opts.snr_thresholds_db) {
        auto ev_opt = FindIntersectionEV(poly_coeffs, threshold_db, *min_max_ev.first, *min_max_ev.second);
        if (ev_opt) {
            // Populate the map with the result
            dr_result.dr_values_ev[threshold_db] = -(*ev_opt);
        }
    }
    
    CurveData curve_data = {raw_file.GetFilename(), "", signal_ev, snr_db, poly_coeffs.clone(), opts.generated_command};

    return {dr_result, curve_data};
}

} // end of anonymous namespace

ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream) {
    ProcessingResult result;
    
    // 1. Load all RAW files into RawFile objects
    std::vector<RawFile> raw_files;
    raw_files.reserve(opts.input_files.size());
    for(const auto& filename : opts.input_files) {
        raw_files.emplace_back(filename);
        if (!raw_files.back().Load()) {
            log_stream << "Error: Could not load RAW file: " << filename << std::endl;
        }
    }
    
    log_stream << "  - Calculating Keystone parameters..." << std::endl;
    std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
    double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
    double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
    std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
    Eigen::VectorXd k = CalculateKeystoneParams(xu, xd);
    log_stream << "  - Keystone parameters calculated." << std::endl;
    
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    // 2. Process each RawFile object
    for (const auto& raw_file : raw_files) {
        if (!raw_file.IsLoaded()) continue;

        auto file_result = AnalyzeSingleRawFile(raw_file, opts, k, log_stream);
        if (!file_result.dr_result.filename.empty()) {
            file_result.curve_data.camera_model = camera_model_name;
            result.dr_results.push_back(file_result.dr_result);
            result.curve_data.push_back(file_result.curve_data);
        }
    }
    return result;
}