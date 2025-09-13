// File: core/ImageProcessing.hpp
#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include <Eigen/Dense>

/**
 * @file ImageProcessing.hpp
 * @brief Declares functions for geometric image processing tasks.
 */

/**
 * @brief Calculates keystone correction parameters from four source and destination points.
 * @param src_points A vector of 4 points from the source (distorted) image.
 * @param dst_points A vector of 4 corresponding points for the destination (corrected) image.
 * @return An Eigen vector containing the 8 transformation parameters.
 */
Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points);

/**
 * @brief Applies an inverse keystone correction to an image.
 * @param imgSrc The source (distorted) image as a cv::Mat.
 * @param k The 8 keystone parameters calculated by CalculateKeystoneParams.
 * @return The corrected image as a new cv::Mat.
 */
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k);
