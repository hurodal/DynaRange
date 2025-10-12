// File: src/core/graphics/ImageProcessing.hpp
/**
 * @file src/core/graphics/ImageProcessing.hpp
 * @brief Declares functions for high-level image processing orchestration and utilities.
 */
#pragma once

#include "../io/raw/RawFile.hpp"
#include "../setup/ChartProfile.hpp"
#include "../arguments/ArgumentsOptions.hpp"
#include "../analysis/Analysis.hpp" // For DataSource
#include <opencv2/core.hpp>
#include <Eigen/Dense>
#include <map>

cv::Mat NormalizeRawImage(const cv::Mat& raw_image, double black_level, double sat_level);
cv::Mat CreateFinalDebugImage(const cv::Mat& overlay_image, double max_pixel_value);

cv::Mat PrepareChartImage(
    const RawFile& raw_file,
    const ProgramOptions& opts,
    const Eigen::VectorXd& keystone_params,
    const ChartProfile& chart,
    std::ostream& log_stream,
    DataSource channel_to_extract
);

/**
 * @brief Draws cross markers on an image at specified corner locations.
 * @param image The source image to draw on.
 * @param corners A vector of 4 corner points.
 * @return A new image with the markers drawn on it.
 */
cv::Mat DrawCornerMarkers(const cv::Mat& image, const std::vector<cv::Point2d>& corners);

/**
 * @brief Prepares all four Bayer channels from a single RAW file in one pass.
 * @details This optimized function loads and normalizes the RAW image once,
 * extracts all four channels (R, G1, G2, B), and then applies keystone
 * correction and cropping to each.
 * @param raw_file The source RawFile object.
 * @param opts The program options (for black/saturation levels).
 * @param keystone_params The pre-calculated keystone transformation parameters.
 * @param chart The chart profile defining the geometry.
 * @param log_stream Stream for logging messages.
 * @return A map where the key is the DataSource (R, G1, G2, B) and the value
 * is the fully prepared cv::Mat for that channel.
 */
std::map<DataSource, cv::Mat> PrepareAllBayerChannels(
    const RawFile& raw_file,
    const ProgramOptions& opts,
    const Eigen::VectorXd& keystone_params,
    const ChartProfile& chart,
    std::ostream& log_stream
);