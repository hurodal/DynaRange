/**
 * @file core/engine/Processing.cpp
 * @brief Implements the core logic for processing and analyzing RAW files.
 */
#include "Processing.hpp"
#include "../Math.hpp"
#include "../graphics/Plotting.hpp"
#include "../RawFile.hpp" 
#include <Eigen/Dense>
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

// Anonymous namespace for internal helper functions.
namespace {

/**
 * @struct SnrCurve
 * @brief Holds the data representing a calculated Signal-to-Noise Ratio curve.
 */
struct SnrCurve {
    std::vector<double> signal_ev; ///< Signal values in EV.
    std::vector<double> snr_db;    ///< Signal-to-Noise ratio values in dB.
    cv::Mat poly_coeffs;           ///< Coefficients of the polynomial fit.
};

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
 * @brief Calculates the keystone correction parameters from hardcoded points.
 * @param log_stream Stream for logging messages.
 * @return An Eigen::VectorXd containing the keystone parameters.
 */
Eigen::VectorXd CalculateKeystone(std::ostream& log_stream) {
    log_stream << "  - Calculating Keystone parameters..." << std::endl;
    std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
    double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
    double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
    std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
    Eigen::VectorXd k = CalculateKeystoneParams(xu, xd);
    log_stream << "  - Keystone parameters calculated." << std::endl;
    return k;
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
 * @brief Calculates SNR and EV values from patch data and fits a polynomial curve.
 * @param patch_data The result of the patch analysis.
 * @param poly_order The order of the polynomial to fit.
 * @return An SnrCurve struct containing the calculated curve data.
 */
SnrCurve CalculateSnrCurve(const PatchAnalysisResult& patch_data, int poly_order) {
    SnrCurve curve;
    for (size_t j = 0; j < patch_data.signal.size(); ++j) {
        curve.snr_db.push_back(20 * log10(patch_data.signal[j] / patch_data.noise[j]));
        curve.signal_ev.push_back(log2(patch_data.signal[j]));
    }

    cv::Mat signal_mat_global(curve.signal_ev.size(), 1, CV_64F, curve.signal_ev.data());
    cv::Mat snr_mat_global(curve.snr_db.size(), 1, CV_64F, curve.snr_db.data());
    
    PolyFit(signal_mat_global, snr_mat_global, curve.poly_coeffs, poly_order);
    return curve;
}

/**
 * @brief Calculates the dynamic range values for a set of thresholds.
 * @param snr_curve The calculated SNR curve.
 * @param thresholds_db The vector of SNR thresholds in dB.
 * @return A map of threshold to calculated dynamic range in EV.
 */
std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_values_ev;
    if (snr_curve.signal_ev.empty()) {
        return dr_values_ev;
    }

    auto min_max_ev = std::minmax_element(snr_curve.signal_ev.begin(), snr_curve.signal_ev.end());

    for (const double threshold_db : thresholds_db) {
        auto ev_opt = FindIntersectionEV(snr_curve.poly_coeffs, threshold_db, *min_max_ev.first, *min_max_ev.second);
        if (ev_opt) {
            dr_values_ev[threshold_db] = -(*ev_opt);
        }
    }
    return dr_values_ev;
}

/**
 * @brief (Orchestrator) Analyzes a single RAW file by calling specialized functions.
 * @param raw_file The RawFile object to be analyzed.
 * @param opts The program options.
 * @param k The keystone correction parameters.
 * @param log_stream The output stream for logging.
 * @return A SingleFileResult struct containing the analysis results.
 */
SingleFileResult AnalyzeSingleRawFile(const RawFile& raw_file, const ProgramOptions& opts, const Eigen::VectorXd& k, std::ostream& log_stream) {
    log_stream << "\nProcessing \"" << fs::path(raw_file.GetFilename()).filename().string() << "\"..." << std::endl;

    cv::Mat img_prepared = PrepareImageForAnalysis(raw_file, opts, k, log_stream);
    if (img_prepared.empty()) {
        return {};
    }

    PatchAnalysisResult patch_data = AnalyzePatches(img_prepared.clone(), 11, 7, opts.patch_ratio);
    if (patch_data.signal.empty()) {
        log_stream << "Warning: No valid patches found for " << raw_file.GetFilename() << std::endl;
        return {};
    }

    SnrCurve snr_curve = CalculateSnrCurve(patch_data, opts.poly_order);
    
    DynamicRangeResult dr_result;
    dr_result.filename = raw_file.GetFilename();
    dr_result.patches_used = (int)patch_data.signal.size();
    dr_result.dr_values_ev = CalculateDynamicRange(snr_curve, opts.snr_thresholds_db);
    
    CurveData curve_data = {
        raw_file.GetFilename(), 
        "", // camera_model is set in ProcessFiles
        snr_curve.signal_ev, 
        snr_curve.snr_db, 
        snr_curve.poly_coeffs.clone(), 
        opts.generated_command
    };

    return {dr_result, curve_data};
}

} // end of anonymous namespace


ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream) {
    ProcessingResult result;
    
    // 1. Load files (Single Responsibility)
    std::vector<RawFile> raw_files = LoadRawFiles(opts.input_files, log_stream);
    
    // 2. Calculate global parameters (Single Responsibility)
    Eigen::VectorXd k = CalculateKeystone(log_stream);
    
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    // 3. Orchestrate analysis for each file (Main Responsibility)
    for (const auto& raw_file : raw_files) {
        if (!raw_file.IsLoaded()) continue;

        auto file_result = AnalyzeSingleRawFile(raw_file, opts, k, log_stream);
        
        // Aggregate valid results
        if (!file_result.dr_result.filename.empty()) {
            file_result.curve_data.camera_model = camera_model_name;
            result.dr_results.push_back(file_result.dr_result);
            result.curve_data.push_back(file_result.curve_data);
        }
    }
    return result;
}