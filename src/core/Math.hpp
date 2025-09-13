// File: core/Math.hpp
#pragma once

#include <opencv2/core.hpp>
#include <vector>
#include <optional>

/**
 * @file Math.hpp
 * @brief Declares standalone mathematical and statistical utility functions.
 */

/**
 * @brief Performs a polynomial fit to a set of 2D points.
 * @param src_x Matrix of independent variable values (x-coordinates).
 * @param src_y Matrix of dependent variable values (y-coordinates).
 * @param dst Output matrix for the calculated polynomial coefficients.
 * @param order The order of the polynomial to fit.
 */
void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order);

/**
 * @brief Finds the intersection of a polynomial curve with a target y-value.
 * @param coeffs The polynomial coefficients.
 * @param target_snr_db The target y-value (SNR in dB) for the intersection.
 * @param min_ev The minimum x-value of the search range.
 * @param max_ev The maximum x-value of the search range.
 * @return An optional containing the x-value (EV) of the intersection, or nullopt if not found.
 */
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);

/**
 * @brief Calculates the arithmetic mean of a vector of doubles.
 * @param data The vector of data points.
 * @return The mean value.
 */
double CalculateMean(const std::vector<double>& data);

/**
 * @brief Calculates a specific quantile from a vector of doubles.
 * @param data The vector of data points. Note: this vector may be reordered.
 * @param percentile The desired percentile (e.g., 0.5 for the median).
 * @return The value at the specified quantile.
 */
double CalculateQuantile(std::vector<double>& data, double percentile);
