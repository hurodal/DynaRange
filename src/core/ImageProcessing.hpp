/**
 * @file core/ImageProcessing.hpp
 * @brief Declares functions for geometric image processing tasks.
 */
#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include <Eigen/Dense>

Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points);
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k);