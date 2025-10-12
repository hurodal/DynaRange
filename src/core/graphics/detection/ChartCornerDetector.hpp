// File: src/core/graphics/detection/ChartCornerDetector.hpp
/**
 * @file ChartCornerDetector.hpp
 * @brief Declares a handler for detecting chart corners in an image.
 * @details This module encapsulates the specific algorithm for finding the
 * four white circular markers on the test chart.
 */
#pragma once

#include <opencv2/core.hpp>
#include <vector>
#include <optional>
#include <ostream>

namespace DynaRange::Graphics::Detection {

/**
 * @brief Detects the four corner points of the test chart from a single-channel Bayer image.
 * @param bayer_image The input single-channel image.
 * @param log_stream The output stream for logging messages.
 * @return An optional containing a vector of 4 corner points (TL, BL, BR, TR) on success, or std::nullopt on failure.
 */
std::optional<std::vector<cv::Point2d>> DetectChartCorners(const cv::Mat& bayer_image, std::ostream& log_stream);

} // namespace DynaRange::Graphics::Detection