// File: src/core/analysis/ImageAnalyzer.cpp
/**
 * @file src/core/analysis/ImageAnalyzer.cpp
 * @brief Implements the patch detection and measurement logic for chart analysis.
 */
#include "ImageAnalyzer.hpp"
#include "Constants.hpp"
#include "../../core/DebugConfig.hpp"
#include <opencv2/imgproc.hpp>

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio, bool create_overlay_image, double min_snr_db, double dark_value) {
    cv::Mat image_with_overlays;
    if (create_overlay_image) {
        image_with_overlays = imgcrop.clone();
    }

    const double patch_width_float = static_cast<double>(imgcrop.cols) / NCOLS;
    const double patch_height_float = static_cast<double>(imgcrop.rows) / NROWS;
    const double safe_x = patch_width_float * (1.0 - patch_ratio) / 2.0;
    const double safe_y = patch_height_float * (1.0 - patch_ratio) / 2.0;

    std::vector<double> signal;
    std::vector<double> noise;
    double max_pixel_value = 0.0;
    signal.reserve(NCOLS * NROWS);
    noise.reserve(NCOLS * NROWS);

    for (int j = 0; j < NROWS; j++) {
        for (int i = 0; i < NCOLS; i++) {
            int x1 = round(static_cast<double>(i) * patch_width_float + safe_x);
            int x2 = round(static_cast<double>(i + 1) * patch_width_float - safe_x);
            int y1 = round(static_cast<double>(j) * patch_height_float + safe_y);
            int y2 = round(static_cast<double>(j + 1) * patch_height_float - safe_y);
            if (x1 >= x2 || y1 >= y2) continue;
            cv::Rect roi_rect(x1, y1, x2 - x1, y2 - y1);
            if (roi_rect.x < 0 || roi_rect.y < 0 || roi_rect.x + roi_rect.width > imgcrop.cols || roi_rect.y + roi_rect.height > imgcrop.rows) continue;
            
            cv::Mat roi = imgcrop(roi_rect);

            cv::Scalar mean_val, stddev_val;
            cv::meanStdDev(roi, mean_val, stddev_val);
            double S = mean_val[0];
            double N = stddev_val[0];

            // --- NEW ROBUST FILTER FOR ZERO BLACK LEVEL SENSORS ---
            // A patch is discarded only if the sensor has zero black level AND
            // the patch itself has zero standard deviation (i.e., it's a solid color block).
            if (dark_value == 0.0 && N == 0.0) {
                continue;
            }
            // --- END OF NEW FILTER ---

            int sat_count = cv::countNonZero(roi > 0.9);
            double sat_ratio = static_cast<double>(sat_count) / (roi.rows * roi.cols);
            
            if (S > 0 && N > 0 && 20 * log10(S / N) >= min_snr_db && sat_ratio < DynaRange::Analysis::Constants::MAX_SATURATION_RATIO) {
                signal.push_back(S);
                noise.push_back(N);
                max_pixel_value = std::max(max_pixel_value, S);

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