// core/engine.cpp
#include "engine.hpp"
#include "functions.hpp"
#include "spline.h"
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

bool run_dynamic_range_analysis(const ProgramOptions& opts, std::ostream& log_stream) {
    const int NCOLS = 11;
    const int NROWS = 7;
    const double SAFE = 50.0;

    std::vector<DynamicRangeResult> all_results;
    Eigen::VectorXd k;
    
    const auto& filenames = opts.input_files;

    for (int i = 0; i < filenames.size(); ++i) {
        const std::string& name = filenames[i];
        log_stream << "\nProcessing \"" << name << "\"..." << std::endl;

        LibRaw raw_processor;
        if (raw_processor.open_file(name.c_str()) != LIBRAW_SUCCESS) {
            log_stream << "Error: Could not open RAW file: " << name << std::endl;
            return false;
        }
        if (raw_processor.unpack() != LIBRAW_SUCCESS) {
            log_stream << "Error: Could not decode RAW data from: " << name << std::endl;
            return false;
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
            std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
            double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
            double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
            std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
            k = calculate_keystone_params(xu, xd);
            log_stream << "  - Keystone parameters calculated." << std::endl;
        }

        cv::Mat imgc = undo_keystone(imgBayer, k);
        double xtl = (119.0 + 99.0) / 2.0; double ytl = (170.0 + 158.0) / 2.0;
        double xbr = (2515.0 + 2473.0) / 2.0; double ybr = (1687.0 + 1679.0) / 2.0;
        cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
        cv::Mat imgcrop = imgc(crop_area);
        PatchAnalysisResult patch_data = analyze_patches(imgcrop.clone(), NCOLS, NROWS, SAFE);
        
        if (patch_data.signal.empty()) {
            log_stream << "Warning: No valid patches found for " << name << std::endl;
            continue;
        }

        std::vector<double> Signal = patch_data.signal;
        std::vector<double> Noise = patch_data.noise;
        std::vector<size_t> p(Signal.size());
        std::iota(p.begin(), p.end(), 0);
        std::sort(p.begin(), p.end(), [&](size_t i, size_t j){ return (Signal[i]/Noise[i]) < (Signal[j]/Noise[j]); });
        
        std::vector<double> sorted_Signal(Signal.size()), sorted_Noise(Signal.size());
        for(size_t idx = 0; idx < Signal.size(); ++idx) {
            sorted_Signal[idx] = Signal[p[idx]]; 
            sorted_Noise[idx] = Noise[p[idx]];
        }
        Signal = sorted_Signal;
        Noise = sorted_Noise;

        std::vector<double> snr_db, signal_ev;
        for (size_t j = 0; j < Signal.size(); ++j) {
            snr_db.push_back(20 * log10(Signal[j] / Noise[j]));
            signal_ev.push_back(log2(Signal[j]));
        }

        tk::spline s;
        s.set_points(snr_db, signal_ev);
        double dr_12db = -s(12.0);
        double dr_0db = -s(0.0);
        
        // --- DEBUGGING LINES ---
        //log_stream << "  - DEBUG: Calculated DR (12dB): " << dr_12db 
        //           << ", DR (0dB): " << dr_0db 
        //           << ", Patches used: " << Signal.size() << std::endl;
        // --- END DEBUGGING LINES ---
        
        all_results.push_back({name, dr_12db, dr_0db, (int)Signal.size()});
    }

    log_stream << "\n--- Dynamic Range Results ---\n";
    log_stream << std::left << std::setw(35) << "RAW File"
              << std::setw(15) << "DR (12dB)" << std::setw(15) << "DR (0dB)"
              << "Patches" << std::endl;
    log_stream << std::string(80, '-') << std::endl;

    for (const auto& res : all_results) {
        log_stream << std::left << std::setw(35) << fs::path(res.filename).filename().string()
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_12db
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_0db
                  << res.patches_used << std::endl;
    }
    
    std::ofstream csv_file(opts.output_filename);
    csv_file << "raw_file,DR_EV_12dB,DR_EV_0dB,patches_used\n";
    for (const auto& res : all_results) {
        csv_file << fs::path(res.filename).filename().string() << "," << res.dr_12db << "," << res.dr_0db << "," << res.patches_used << "\n";
    }
    csv_file.close();
    log_stream << "\nResults saved to " << opts.output_filename << std::endl;

    return true;
}