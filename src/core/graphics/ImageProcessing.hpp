// File: src/core/graphics/ImageProcessing.hpp
/**
 * @file src/core/graphics/ImageProcessing.hpp
 * @brief Declares functions for geometric image processing tasks.
 */
#pragma once

#include "RawFile.hpp"
#include "../ChartProfile.hpp"
#include "Arguments.hpp"
#include <vector>
#include <opencv2/core.hpp>
#include <Eigen/Dense>

/**
 * @brief Calculates keystone distortion parameters from four pairs of points.
 * @param src_points The four corner points in the distorted source image.
 * @param dst_points The corresponding four corner points in the target rectified image.
 * @return An Eigen::VectorXd containing the 8 transformation parameters.
 */
Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points);

/**
 * @brief Applies an inverse keystone correction to an image.
 * @param imgSrc The source image to be corrected.
 * @param k An Eigen::VectorXd containing the 8 transformation parameters.
 * @return A new cv::Mat containing the rectified image.
 */
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k);

/**
 * @brief Prepares a chart image for analysis.
 * @details Performs all necessary steps: normalization, Bayer channel extraction,
 * keystone correction, and cropping based on a chart profile.
 * @param raw_file The source RawFile object.
 * @param opts The program options (for black/saturation levels).
 * @param chart The chart profile defining the geometry.
 * @param log_stream Stream for logging messages.
 * @return A prepared cv::Mat, ready for patch analysis. Returns empty Mat on failure.
 */
cv::Mat PrepareChartImage(const RawFile& raw_file, const ProgramOptions& opts, const ChartProfile& chart, std::ostream& log_stream);