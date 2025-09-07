// Fichero: core/Engine.cpp
#include "Engine.hpp"
#include "Analysis.hpp"
#include "Plotting.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <libraw/libraw.h>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

namespace fs = std::filesystem;

double CalculateDrForThreshold(double threshold, const std::vector<double>& snr_db, const std::vector<double>& signal_ev, int poly_order, std::ostream& log_stream) {
    const double filter_range = 5.0;
    std::vector<double> filtered_snr, filtered_signal;
    for (size_t i = 0; i < snr_db.size(); ++i) {
        if (snr_db[i] >= (threshold - filter_range) && snr_db[i] <= (threshold + filter_range)) {
            filtered_snr.push_back(snr_db[i]);
            filtered_signal.push_back(signal_ev[i]);
        }
    }
    log_stream << "  - Info: For " << threshold << "dB threshold, using " << filtered_snr.size() << " patches." << std::endl;
    if (filtered_snr.size() < (size_t)poly_order + 1) {
        log_stream << "  - Warning: Not enough data points for polynomial fit (order " << poly_order << ") at " << threshold << "dB." << std::endl;
        return 0.0;
    }
    cv::Mat snr_mat(filtered_snr.size(), 1, CV_64F, filtered_snr.data());
    cv::Mat signal_mat(filtered_signal.size(), 1, CV_64F, filtered_signal.data());
    cv::Mat coeffs;
    PolyFit(snr_mat, signal_mat, coeffs, poly_order);
    double signal_at_threshold = 0.0;
    for (int i = 0; i < coeffs.rows; ++i) {
        signal_at_threshold += coeffs.at<double>(i) * std::pow(threshold, coeffs.rows - 1 - i);
    }
    return -signal_at_threshold;
}

std::optional<std::string> RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {
    // 1. Procesar dark y saturation frames (si se proporcionaron)
    if (!opts.dark_file_path.empty()) {
        auto dark_val_opt = ProcessDarkFrame(opts.dark_file_path, log_stream);
        if (!dark_val_opt) { log_stream << "Fatal error processing dark frame." << std::endl; return std::nullopt; }
        opts.dark_value = *dark_val_opt;
    }
    if (!opts.sat_file_path.empty()) {
        auto sat_val_opt = ProcessSaturationFrame(opts.sat_file_path, log_stream);
        if (!sat_val_opt) { log_stream << "Fatal error processing saturation frame." << std::endl; return std::nullopt; }
        opts.saturation_value = *sat_val_opt;
    }

    // 2. Imprimir la configuración final
    log_stream << std::fixed << std::setprecision(2);
    log_stream << "\n[FINAL CONFIGURATION]\n";
    log_stream << "Black level: " << opts.dark_value << "\n";
    log_stream << "Saturation point: " << opts.saturation_value << "\n";
    log_stream << "SNR threshold: " << opts.snr_threshold_db << " dB\n";
    log_stream << "DR normalization: " << opts.dr_normalization_mpx << " Mpx\n";
    log_stream << "Polynomic order: " << opts.poly_order << "\n";
    log_stream << "Patch safe: " << opts.patch_safe << " px\n";
    log_stream << "Output file: " << opts.output_filename << "\n\n";

    // 3. Preparar y ordenar ficheros
    if (!PrepareAndSortFiles(opts, log_stream)) {
        return std::nullopt;
    }

    // 4. Continuar con el análisis principal...
    const int NCOLS = 11, NROWS = 7;
    const double SAFE = static_cast<double>(opts.patch_safe); 
    
    std::vector<DynamicRangeResult> all_results;
    std::vector<CurveData> all_curves_data;
    Eigen::VectorXd k;
    const auto& filenames = opts.input_files;
    
    std::string camera_model_name = "";

    for (size_t i = 0; i < filenames.size(); ++i) {
        const std::string& name = filenames[i];
        log_stream << "\nProcessing \"" << fs::path(name).filename().string() << "\"..." << std::endl;

        if (i == 0) {
            camera_model_name = GetCameraModel(name);
            if (!camera_model_name.empty()) {
                log_stream << "  - Info: Detected camera model: " << camera_model_name << std::endl;
            }
        }

        LibRaw raw_processor;
        if (raw_processor.open_file(name.c_str()) != LIBRAW_SUCCESS || raw_processor.unpack() != LIBRAW_SUCCESS) {
            log_stream << "Error: Could not open/decode RAW file: " << name << std::endl;
            continue;
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
        if (i == 0) {
            log_stream << "  - Calculating Keystone parameters..." << std::endl;
            std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
            double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
            double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
            std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
            k = CalculateKeystoneParams(xu, xd);
            log_stream << "  - Keystone parameters calculated." << std::endl;
        }
        cv::Mat imgc = UndoKeystone(imgBayer, k);
        double xtl = (119.0 + 99.0) / 2.0; double ytl = (170.0 + 158.0) / 2.0;
        double xbr = (2515.0 + 2473.0) / 2.0; double ybr = (1687.0 + 1679.0) / 2.0;
        cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
        cv::Mat imgcrop = imgc(crop_area);
        PatchAnalysisResult patch_data = AnalyzePatches(imgcrop.clone(), NCOLS, NROWS, SAFE);
        if (patch_data.signal.empty()) {
            log_stream << "Warning: No valid patches found for " << name << std::endl;
            continue;
        }
        std::vector<double> snr_db, signal_ev;
        for (size_t j = 0; j < patch_data.signal.size(); ++j) {
            snr_db.push_back(20 * log10(patch_data.signal[j] / patch_data.noise[j]));
            signal_ev.push_back(log2(patch_data.signal[j]));
        }
        cv::Mat signal_mat_global(signal_ev.size(), 1, CV_64F, signal_ev.data());
        cv::Mat snr_mat_global(snr_db.size(), 1, CV_64F, snr_db.data());
        
        cv::Mat poly_coeffs_for_drawing;
        PolyFit(signal_mat_global, snr_mat_global, poly_coeffs_for_drawing, opts.poly_order);
        
        cv::Mat poly_coeffs_for_intersection;
        PolyFit(snr_mat_global, signal_mat_global, poly_coeffs_for_intersection, 2);

        fs::path plot_path = fs::path(opts.output_filename).parent_path() / (fs::path(name).stem().string() + "_snr_plot.png");
        
        GenerateSnrPlot(plot_path.string(), fs::path(name).filename().string(), signal_ev, snr_db, poly_coeffs_for_drawing, poly_coeffs_for_intersection, log_stream);
        
        double dr_12db = CalculateDrForThreshold(opts.snr_threshold_db, snr_db, signal_ev, 2, log_stream);
        double dr_0db = CalculateDrForThreshold(0.0, snr_db, signal_ev, 2, log_stream);
        all_results.push_back({name, dr_12db, dr_0db, (int)patch_data.signal.size()});
        
        all_curves_data.push_back({name, camera_model_name, signal_ev, snr_db, poly_coeffs_for_drawing.clone(), poly_coeffs_for_intersection.clone()});
    }

    std::optional<std::string> summary_plot_path_opt = std::nullopt;
    if (!all_curves_data.empty()) {
        fs::path output_dir_path = fs::path(opts.output_filename).parent_path();
        summary_plot_path_opt = GenerateSummaryPlot(output_dir_path.string(), camera_model_name, all_curves_data, log_stream);
    }

    log_stream << "\n--- Dynamic Range Results ---\n";
    std::stringstream dr_header_ss;
    dr_header_ss << "DR(" << std::fixed << std::setprecision(2) << opts.snr_threshold_db << "dB)";
    log_stream << std::left << std::setw(30) << "RAW File" << std::setw(20) << dr_header_ss.str() << std::setw(15) << "DR(0dB)" << "Patches" << std::endl;
    log_stream << std::string(80, '-') << std::endl;
    for (const auto& res : all_results) {
        log_stream << std::left << std::setw(30) << fs::path(res.filename).filename().string() << std::fixed << std::setprecision(4) << std::setw(20) << res.dr_12db << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_0db << res.patches_used << std::endl;
    }
    std::ofstream csv_file(opts.output_filename);
    csv_file << "raw_file,DR_" << opts.snr_threshold_db << "dB,DR_0dB,patches_used\n";
    for (const auto& res : all_results) {
        csv_file << fs::path(res.filename).filename().string() << "," << res.dr_12db << "," << res.dr_0db << "," << res.patches_used << "\n";
    }
    csv_file.close();
    log_stream << "\nResults saved to " << opts.output_filename << std::endl;
    
    return summary_plot_path_opt;
}