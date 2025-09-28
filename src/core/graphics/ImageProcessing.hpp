// File: src/core/graphics/ImageProcessing.hpp
/**
 * @file src/core/graphics/ImageProcessing.hpp
 * @brief Declares functions for geometric image processing tasks.
 */
#pragma once

#include "../io/RawFile.hpp"
#include "../setup/ChartProfile.hpp"
#include "../arguments/ProgramOptions.hpp"
#include <opencv2/core.hpp>
#include <Eigen/Dense>

cv::Mat NormalizeRawImage(const cv::Mat& raw_image, double black_level, double sat_level);
Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points);

/**
 * @brief Applies an inverse keystone correction to a single-channel float image.
 * @param imgSrc The source image (CV_32FC1) to be corrected.
 * @param k An Eigen::VectorXd containing the 8 transformation parameters.
 * @return A new cv::Mat containing the rectified image.
 */
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k);

/**
 * @brief (New Function) Applies an inverse keystone correction to a 3-channel color image.
 * @param imgSrc The source image (CV_8UC3) to be corrected.
 * @param k An Eigen::VectorXd containing the 8 transformation parameters.
 * @return A new cv::Mat containing the rectified color image.
 */
cv::Mat UndoKeystoneColor(const cv::Mat& imgSrc, const Eigen::VectorXd& k);

cv::Mat CreateFinalDebugImage(const cv::Mat& overlay_image, double max_pixel_value);
cv::Mat PrepareChartImage(
    const RawFile& raw_file, 
    const ProgramOptions& opts, 
    const Eigen::VectorXd& keystone_params,
    const ChartProfile& chart, 
    std::ostream& log_stream);