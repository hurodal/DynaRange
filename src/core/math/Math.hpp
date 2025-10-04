// File: src/core/math/Math.hpp
/**
 * @file src/core/math/Math.hpp
 * @brief Declares standalone mathematical and statistical utility functions.
 */
#pragma once
#include <opencv2/core.hpp>
#include <vector>

/**
 * @brief Evaluates a polynomial at a given point.
 * @param coeffs A cv::Mat (CV_64F) of size (order+1)x1 containing the polynomial coefficients.
 * @param x The value at which to evaluate the polynomial.
 * @return The calculated value of the polynomial P(x).
 */
double EvaluatePolynomial(const cv::Mat& coeffs, double x);

/**
 * @brief Evaluates the derivative of a polynomial at a given point.
 * @param coeffs A cv::Mat (CV_64F) of size (order+1)x1 containing the polynomial coefficients.
 * @param x The value at which to evaluate the derivative.
 * @return The calculated value of the derivative P'(x).
 */
double EvaluatePolynomialDerivative(const cv::Mat& coeffs, double x);

/**
 * @brief Fits a polynomial of a specified order to a set of 2D points.
 * @param src_x A cv::Mat (CV_64F) of size Nx1 containing the x-coordinates.
 * @param src_y A cv::Mat (CV_64F) of size Nx1 containing the y-coordinates.
 * @param dst A cv::Mat that will contain the output polynomial coefficients.
 * @param order The order of the polynomial to fit.
 */
void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order);
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