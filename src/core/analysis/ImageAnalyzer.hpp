// File: src/core/analysis/ImageAnalyzer.hpp
/**
 * @file src/core/analysis/ImageAnalyzer.hpp
 * @brief Declares the function to analyze chart patches in a cropped image.
 */
#pragma once

#include <opencv2/core.hpp>
#include "../analysis/Analysis.hpp"

/**
 * @brief Analyzes a cropped chart image to find patches and measure their signal and noise.
 * @param imgcrop The input image, corrected for geometry and cropped to the chart area.
 * @param NCOLS The number of columns in the patch grid.
 * @param NROWS The number of rows in the patch grid.
 * @param patch_ratio The relative area of the center of each patch to sample.
 * @param create_overlay_image If true, an image with patch overlays will be generated.
 * @return A PatchAnalysisResult struct containing the signal and noise vectors.
 */
PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio, bool create_overlay_image);