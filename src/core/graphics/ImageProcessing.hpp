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

/**
 * @brief Normalizes a raw image to a [0.0, 1.0] float range.
 * @details This is a pure processing function, taking raw data and calibration
 * values to produce a normalized image.
 * @param raw_image The 16-bit raw image data from the sensor.
 * @param black_level The black level to subtract.
 * @param sat_level The saturation level to use for normalization.
 * @return A 32-bit floating-point cv::Mat. Returns an empty Mat on failure.
 */
cv::Mat NormalizeRawImage(const cv::Mat& raw_image, double black_level, double sat_level);

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
 * @param keystone_params The pre-calculated keystone transformation parameters.
 * @param chart The chart profile defining the geometry.
 * @param log_stream Stream for logging messages.
 * @return A prepared cv::Mat, ready for patch analysis.
 * Returns empty Mat on failure.
 */
cv::Mat PrepareChartImage(
    const RawFile& raw_file, 
    const ProgramOptions& opts, 
    const Eigen::VectorXd& keystone_params,
    const ChartProfile& chart, 
    std::ostream& log_stream);