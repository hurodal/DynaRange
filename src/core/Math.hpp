// File: core/Math.hpp
#pragma once

#include <opencv2/core.hpp>
#include <vector>
#include <optional>

// Performs a polynomial fit to a set of 2D points.
void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order);

// Finds the intersection of a polynomial curve (SNR = f(EV)) with a target SNR value.
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);