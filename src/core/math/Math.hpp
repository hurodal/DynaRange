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
 * @brief Calculates the Exposure Value (EV) for a given Signal-to-Noise Ratio (SNR).
 * @details This function evaluates a polynomial of the form EV = f(SNR_dB).
 * It directly computes the EV by substituting the target_snr_db into the polynomial,
 * which is more efficient than solving for a root. The polynomial order is
 * determined by the number of coefficients.
 * @param coeffs The polynomial coefficients, as calculated by PolyFit for an EV = f(SNR_dB) model.
 * @param target_snr_db The input SNR value (in dB) at which to evaluate the polynomial.
 * @return An std::optional containing the calculated EV value. Returns std::nullopt if the 
 * coefficient matrix is empty.
 */
std::optional<double> CalculateEVFromSNR(const cv::Mat& coeffs, double target_snr_db);

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