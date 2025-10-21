// File: src/core/graphics/ImageProcessing.hpp
/**
 * @file src/core/graphics/ImageProcessing.hpp
 * @brief Declares functions for high-level image processing orchestration and utilities.
 */
#pragma once

#include "../io/raw/RawFile.hpp"
#include "../setup/ChartProfile.hpp"
#include "../analysis/Analysis.hpp" // For DataSource
#include "../utils/PathManager.hpp"
#include <opencv2/core.hpp>
#include <map>
#include <string> // Added for camera_model_name

cv::Mat NormalizeRawImage(const cv::Mat& raw_image, double black_level, double sat_level);
/**
 * @brief Creates the final, viewable debug image from the overlay data.
 * @details Applies consistent visualization processing (contrast stretch, gamma)
 * to the input image which already contains overlays.
 * @param overlay_image The single-channel image (CV_32F) with patch outlines drawn on it.
 * @param max_pixel_value The maximum signal value from valid patches (currently unused).
 * @return A gamma-corrected cv::Mat (CV_32F BGR, range 0.0-1.0) ready for saving.
 */
cv::Mat CreateFinalDebugImage(const cv::Mat& overlay_image, double max_pixel_value);
/**
 * @brief Prepares a single-channel Bayer image for analysis.
 * @param raw_file The source RawFile object.
 * @param dark_value The black level for normalization.
 * @param saturation_value The saturation level for normalization.
 * @param keystone_params The pre-calculated keystone transformation parameters.
 * @param chart The chart profile defining the geometry.
 * @param log_stream Stream for logging messages.
 * @param channel_to_extract The specific Bayer channel to extract.
 * @param paths The PathManager for resolving debug output paths.
 * @param camera_model_name The camera model name (for debug filenames). // *** NUEVO PARÁMETRO ***
 * @return A fully prepared cv::Mat for the specified channel.
 */
cv::Mat PrepareChartImage(
    const RawFile& raw_file,
    double dark_value,
    double saturation_value,
    const cv::Mat& keystone_params,
    const ChartProfile& chart,
    std::ostream& log_stream,
    DataSource channel_to_extract,
    const PathManager& paths,
    const std::string& camera_model_name,
    bool generate_full_debug
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
 * @param raw_file The source RawFile object.
 * @param dark_value The black level for normalization.
 * @param saturation_value The saturation level for normalization.
 * @param keystone_params The pre-calculated keystone transformation parameters.
 * @param chart The chart profile defining the geometry.
 * @param log_stream Stream for logging messages.
 * @return A map where the key is the DataSource (R, G1, G2, B) and the value
 * is the fully prepared cv::Mat for that channel.
 */
std::map<DataSource, cv::Mat> PrepareAllBayerChannels(
    const RawFile& raw_file,
    double dark_value,
    double saturation_value,
    const cv::Mat& keystone_params,
    const ChartProfile& chart,
    std::ostream& log_stream
);

/**
 * @brief Prepares a single-channel float image for consistent visual debugging display.
 * @details Applies robust contrast stretching based on percentiles and gamma correction.
 * @param img_float The input single-channel image (CV_32F), assumed normalized by black/saturation.
 * @return A 3-channel BGR image (CV_32F, range [0,1]) ready for drawing overlays or saving. Returns empty Mat on error.
 */
cv::Mat PrepareDebugImageView(const cv::Mat& img_float);