// File: src/core/analysis/ImageAnalyzer.cpp
/**
 * @file src/core/analysis/ImageAnalyzer.cpp
 * @brief Implements the patch detection and measurement logic for chart analysis.
 */
#include "ImageAnalyzer.hpp"
#include <opencv2/imgproc.hpp>

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio) {
    std::vector<double> signal_vec, noise_vec;
    const double patch_width = (double)imgcrop.cols / NCOLS;
    const double patch_height = (double)imgcrop.rows / NROWS;
    const double safe_x = patch_width * (1.0 - patch_ratio) / 2.0;
    const double safe_y = patch_height * (1.0 - patch_ratio) / 2.0;

    for (int j = 0; j < NROWS; ++j) {
        for (int i = 0; i < NCOLS; ++i) {
            int x1 = round((double)i * patch_width + safe_x);
            int x2 = round((double)(i + 1) * patch_width - safe_x);
            int y1 = round((double)j * patch_height + safe_y);
            int y2 = round((double)(j + 1) * patch_height - safe_y);

            if (x1 >= x2 || y1 >= y2) continue;

            cv::Mat patch = imgcrop(cv::Rect(x1, y1, x2 - x1, y2 - y1));
            cv::Scalar mean, stddev;
            cv::meanStdDev(patch, mean, stddev);
            double S = mean[0], N = stddev[0];
            int sat_count = cv::countNonZero(patch > 0.9);
            double sat_ratio = (double)sat_count / (patch.rows * patch.cols);

            if (S > 0 && N > 0 && 20 * log10(S / N) >= -10 && sat_ratio < 0.01) {
                signal_vec.push_back(S);
                noise_vec.push_back(N);
                cv::rectangle(imgcrop, {x1, y1}, {x2, y2}, cv::Scalar(0.0), 1);
                cv::rectangle(imgcrop, {x1 - 1, y1 - 1}, {x2 + 1, y2 + 1}, cv::Scalar(1.0), 1);
            }
        }
    }
    return {signal_vec, noise_vec, imgcrop};
}