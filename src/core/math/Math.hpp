// File: src/core/math/Math.hpp
/**
 * @file src/core/math/Math.hpp
 * @brief Declares standalone mathematical and statistical utility functions.
 */
#pragma once

#include <opencv2/core.hpp>
#include <vector>
#include <optional>

/**
 * @brief Fits a polynomial of a specified order to a set of 2D points.
 * @param src_x A cv::Mat (CV_64F) of size Nx1 containing the x-coordinates.
 * @param src_y A cv::Mat (CV_64F) of size Nx1 containing the y-coordinates.
 * @param dst A cv::Mat that will contain the output polynomial coefficients.
 * @param order The order of the polynomial to fit.
 */
void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order);

/**
 * @brief Finds the x-value (EV) where a polynomial curve intersects a target y-value (SNR).
 * @details Solves the equation P(x) = target_snr_db for x within a given range.
 * Supports quadratic (order 2) and cubic (order 3) polynomials.
 * @param coeffs The polynomial coefficients, as calculated by PolyFit.
 * @param target_snr_db The target SNR value in dB to find the intersection for.
 * @param min_ev The minimum bound of the search range for the EV value.
 * @param max_ev The maximum bound of the search range for the EV value.
 * @return An optional containing the EV value at the intersection, or std::nullopt if not found.
 */
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);

/**
 * @brief Calculates the arithmetic mean of a vector of doubles.
 * @param data The vector of numbers.
 * @return The calculated mean, or 0.0 if the vector is empty.
 */
double CalculateMean(const std::vector<double>& data);

/**
 * @brief Calculates a specific quantile from a vector of doubles.
 * @param data The vector of numbers. Note: This vector may be reordered by the function.
 * @param percentile The desired quantile (e.g., 0.5 for the median).
 * @return The value at the specified quantile, or 0.0 if the vector is empty.
 */
double CalculateQuantile(std::vector<double>& data, double percentile);