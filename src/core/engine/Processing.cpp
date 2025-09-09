// Fichero: core/engine/Processing.cpp
#include "Processing.hpp"
#include "../graphics/Plotting.hpp"
#include <libraw/libraw.h>
#include <Eigen/Dense>
#include <filesystem>
#include <iostream>
#include <algorithm> // Para std::minmax_element

namespace fs = std::filesystem;

namespace { // Funciones auxiliares internas a este fichero

// Forward declaration so ProcessFiles can see this function
SingleFileResult AnalyzeSingleRawFile(const std::string& name, const ProgramOptions& opts, const Eigen::VectorXd& k, std::ostream& log_stream);

// Analyzes a single RAW file, extracts patches, fits SNR curve, and calculates DR.
SingleFileResult AnalyzeSingleRawFile(const std::string& name, const ProgramOptions& opts, const Eigen::VectorXd& k, std::ostream& log_stream) {
    log_stream << "\nProcessing \"" << fs::path(name).filename().string() << "\"..." << std::endl;

    LibRaw raw_processor;
    if (raw_processor.open_file(name.c_str()) != LIBRAW_SUCCESS || raw_processor.unpack() != LIBRAW_SUCCESS) {
        log_stream << "Error: Could not open/decode RAW file: " << name << std::endl;
        return {};
    }
    log_stream << "  - Info: Black=" << opts.dark_value << ", Saturation=" << opts.saturation_value << std::endl;
    cv::Mat raw_image(raw_processor.imgdata.sizes.raw_height, raw_processor.imgdata.sizes.raw_width, CV_16U, raw_processor.imgdata.rawdata.raw_image);
    cv::Mat img_float;
    raw_image.convertTo(img_float, CV_32F);
    img_float = (img_float - opts.dark_value) / (opts.saturation_value - opts.dark_value);
    cv::Mat imgBayer(raw_processor.imgdata.sizes.raw_height / 2, raw_processor.imgdata.sizes.raw_width / 2, CV_32FC1);
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
    // MODIFIED: Use opts.patch_ratio instead of opts.patch_safe
    PatchAnalysisResult patch_data = AnalyzePatches(imgcrop.clone(), 11, 7, opts.patch_ratio);
    
    if (patch_data.signal.empty()) {
        log_stream << "Warning: No valid patches found for " << name << std::endl;
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

    fs::path plot_path = fs::path(opts.output_filename).parent_path() / (fs::path(name).stem().string() + "_snr_plot.png");
    GenerateSnrPlot(plot_path.string(), fs::path(name).filename().string(), signal_ev, snr_db, poly_coeffs, opts, log_stream);
    
    auto min_max_ev = std::minmax_element(signal_ev.begin(), signal_ev.end());
    // MODIFIED: Use the first element of the thresholds vector for the primary DR calculation
    double dr_primary = -(*FindIntersectionEV(poly_coeffs, opts.snr_thresholds_db[0], *min_max_ev.first, *min_max_ev.second));
    double dr_0db = -(*FindIntersectionEV(poly_coeffs, 0.0, *min_max_ev.first, *min_max_ev.second));
    
    return {
        {name, dr_primary, dr_0db, (int)patch_data.signal.size()},
        {name, "", signal_ev, snr_db, poly_coeffs.clone(), opts.generated_command}
    };
}

} // fin del namespace anónimo

ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream) {
    ProcessingResult result;
    const auto& filenames = opts.input_files;
    
    log_stream << "  - Calculating Keystone parameters..." << std::endl;
    std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
    double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
    double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
    std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
    Eigen::VectorXd k = CalculateKeystoneParams(xu, xd);
    log_stream << "  - Keystone parameters calculated." << std::endl;
    
    std::string camera_model_name = GetCameraModel(filenames[0]);

    for (const auto& name : filenames) {
        auto file_result = AnalyzeSingleRawFile(name, opts, k, log_stream);
        if (!file_result.dr_result.filename.empty()) {
            file_result.curve_data.camera_model = camera_model_name;
            result.dr_results.push_back(file_result.dr_result);
            result.curve_data.push_back(file_result.curve_data);
        }
    }
    return result;
}