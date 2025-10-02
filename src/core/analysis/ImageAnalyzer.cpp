// File: src/core/analysis/ImageAnalyzer.cpp
/**
 * @file src/core/analysis/ImageAnalyzer.cpp
 * @brief Implements the patch detection and measurement logic for chart analysis.
 */
#include "ImageAnalyzer.hpp"
#include "../../core/DebugConfig.hpp"
#include <opencv2/imgproc.hpp>

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio) {
    const int width = imgcrop.cols / NCOLS;
    const int height = imgcrop.rows / NROWS;
    const int patch_width = static_cast<int>(width * patch_ratio);
    const int patch_height = static_cast<int>(height * patch_ratio);

    std::vector<double> signal;
    std::vector<double> noise;
    double max_pixel_value = 0.0;

    signal.reserve(NCOLS * NROWS);
    noise.reserve(NCOLS * NROWS);

    for (int j = 0; j < NROWS; j++) {
        for (int i = 0; i < NCOLS; i++) {
            const int x = i * width + (width - patch_width) / 2;
            const int y = j * height + (height - patch_height) / 2;
            cv::Rect roi_rect(x, y, patch_width, patch_height);
            
            if (roi_rect.x < 0 || roi_rect.y < 0 || roi_rect.x + roi_rect.width > imgcrop.cols || roi_rect.y + roi_rect.height > imgcrop.rows) continue;
            
            cv::Mat roi = imgcrop(roi_rect);

            cv::Scalar mean_val, stddev_val;
            cv::meanStdDev(roi, mean_val, stddev_val);
            double S = mean_val[0];
            double N = stddev_val[0];
            
            // --- START BLOCK 1 (CONTROL POINT 5) ---
            #if DEBUG_IA_ON == 1
                // Static to print header only once per run
                static bool cp5_header_printed = false;
                if (!cp5_header_printed) {
                    std::cout << "\n--- DEBUG IA: Raw Patch Data (Point 5) ---\n";
                    std::cout << std::setw(10) << "Patch #" << std::setw(20) << "Signal (S)" << std::setw(20) << "Noise (N)" << std::setw(15) << "SNR (dB)" << std::endl;
                    std::cout << "-----------------------------------------------------------------\n";
                    cp5_header_printed = true;
                }

                if (S <= 0.0 || N <= 0.0) {
                    std::cout << std::setw(10) << (j * NCOLS + i) 
                              << std::fixed << std::setprecision(8) << std::setw(20) << S 
                              << std::setw(20) << N
                              << std::setw(15) << "-inf" << std::endl;
                } else {
                    double snr_db = 20.0 * std::log10(S / N);
                    std::cout << std::setw(10) << (j * NCOLS + i) 
                              << std::fixed << std::setprecision(8) << std::setw(20) << S 
                              << std::setw(20) << N
                              << std::fixed << std::setprecision(4) << std::setw(15) << snr_db << std::endl;
                }
            #endif
            // --- END BLOCK 1 ---

            // --- MODIFICATION: Restored the original filtering logic to match the old working version. ---
            int sat_count = cv::countNonZero(roi > 0.9);
            double sat_ratio = (double)sat_count / (roi.rows * roi.cols);

            if (S > 0 && N > 0 && 20 * log10(S / N) >= -10 && sat_ratio < 0.01) {
                signal.push_back(S);
                noise.push_back(N);
                max_pixel_value = std::max(max_pixel_value, S);
            }
        }
    }

    PatchAnalysisResult result;
    result.signal = signal;
    result.noise = noise;
    result.max_pixel_value = max_pixel_value;
    
    // --- START BLOCK 2 (CONTROL POINT 5) ---
    #if DEBUG_IA_ON == 1
        std::cout << "-----------------------------------------------------------------\n";
        std::cout << "--- DEBUG IA: Finalizing program at control point 5. ---\n" << std::endl;
        //exit(0);
    #endif
    // --- END BLOCK 2 ---   
    return result;
}