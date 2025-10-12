// File: src/core/graphics/geometry/KeystoneCorrection.hpp
/**
 * @file KeystoneCorrection.hpp
 * @brief Declares functions for geometric keystone correction.
 * @details This module encapsulates the mathematical logic for calculating
 * keystone transformation parameters and applying the correction to an image.
 */
#pragma once

#include <opencv2/core.hpp>
#include <vector>

namespace DynaRange::Graphics::Geometry {

/**
 * @brief Calculates the 8 parameters for an inverse keystone transformation.
 * @param src_points A vector of 4 source points (the distorted corners).
 * @param dst_points A vector of 4 destination points (the target rectangular corners).
 * @return A cv::Mat containing the 8 transformation parameters (k).
 */
cv::Mat CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points);

/**
 * @brief Applies an inverse keystone correction to a single-channel float image.
 * @param imgSrc The source image (CV_32FC1) to be corrected.
 * @param k A cv::Mat containing the 8 transformation parameters.
 * @return A new cv::Mat containing the rectified image.
 */
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const cv::Mat& k);

} // namespace DynaRange::Graphics::Geometry