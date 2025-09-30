// File: src/core/analysis/ImageAnalyzer.cpp
/**
 * @file src/core/analysis/ImageAnalyzer.cpp
 * @brief Implements the patch detection and measurement logic for chart analysis.
 */
#include "ImageAnalyzer.hpp"
#include "../../core/DebugConfig.hpp"
#include <opencv2/imgproc.hpp>

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio) {
    std::vector<double> signal_vec, noise_vec;
    
    // Find max pixel value for normalization before drawing overlays.
    double max_val = 0.0;
    cv::minMaxLoc(imgcrop, nullptr, &max_val);

    // Se convierte la imagen a color (BGR) para poder dibujar rectángulos de color.
    cv::Mat color_imgcrop;
    cv::cvtColor(imgcrop, color_imgcrop, cv::COLOR_GRAY2BGR);

    #if DYNA_RANGE_DEBUG_MODE == 1
    const cv::Scalar inner_color = cv::Scalar(
        DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[0],
        DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[1],
        DynaRange::Debug::PATCH_OUTLINE_INNER_COLOR[2]
    );
    const cv::Scalar outer_color = cv::Scalar(
        DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[0],
        DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[1],
        DynaRange::Debug::PATCH_OUTLINE_OUTER_COLOR[2]
    );
    #else
    const cv::Scalar inner_color = cv::Scalar(0.0, 0.0, 0.0); // Negro por defecto
    const cv::Scalar outer_color = cv::Scalar(1.0, 1.0, 1.0); // Blanco por defecto
    #endif

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
                
                // Se dibuja el rectángulo interior (negro).
                cv::rectangle(color_imgcrop, {x1, y1}, {x2, y2}, inner_color, 1);
                // Se dibuja el rectángulo exterior (blanco).
                cv::rectangle(color_imgcrop, {x1 - 1, y1 - 1}, {x2 + 1, y2 + 1}, outer_color, 1);
            }
        }
    }
    // Se devuelve la imagen a color con los parches dibujados.
    return {signal_vec, noise_vec, color_imgcrop, max_val};
}