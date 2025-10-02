// File: src/core/analysis/ImageAnalyzer.cpp
/**
 * @file src/core/analysis/ImageAnalyzer.cpp
 * @brief Implements the patch detection and measurement logic for chart analysis.
 */
#include "ImageAnalyzer.hpp"
#include "../../core/DebugConfig.hpp"
#include <opencv2/imgproc.hpp>

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio, bool create_overlay_image) {
    cv::Mat image_with_overlays;
    if (create_overlay_image) {
        image_with_overlays = imgcrop; // The input is already a copy.
    }

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
            
            int sat_count = cv::countNonZero(roi > 0.9);
            double sat_ratio = (double)sat_count / (roi.rows * roi.cols);

            if (S > 0 && N > 0 && 20 * log10(S / N) >= -10 && sat_ratio < 0.01) {
                signal.push_back(S);
                noise.push_back(N);
                max_pixel_value = std::max(max_pixel_value, S);

                // Drawing logic is now conditional.
                if (create_overlay_image) {
                    #if DYNA_RANGE_DEBUG_MODE == 1
                        cv::rectangle(image_with_overlays, roi_rect.tl() - cv::Point(1,1), roi_rect.br() + cv::Point(1,1), 
                                      cv::Scalar(DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[2], DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[1], DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[0]), 1);
                        cv::rectangle(image_with_overlays, roi_rect, 
                                      cv::Scalar(DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[2], DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[1], DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[0]), 1);
                    #else
                        cv::rectangle(image_with_overlays, roi_rect.tl() - cv::Point(1,1), roi_rect.br() + cv::Point(1,1), cv::Scalar(1.0), 1);
                        cv::rectangle(image_with_overlays, roi_rect, cv::Scalar(0.0), 1);
                    #endif
                }
            }
        }
    }

    PatchAnalysisResult result;
    result.signal = signal;
    result.noise = noise;
    result.max_pixel_value = max_pixel_value;
    if (create_overlay_image) {
        result.image_with_patches = image_with_overlays;
    }
    
    return result;
}
