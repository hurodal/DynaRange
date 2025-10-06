// File: src/core/math/Math.cpp
/**
 * @file src/core/math/Math.cpp
 * @brief Implements mathematical and statistical utility functions.
 */
#include "Math.hpp"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <numeric>
#include <algorithm>

double EvaluatePolynomial(const cv::Mat& coeffs, double x) {
    if (coeffs.empty()) {
        return 0.0;
    }
    double result = 0.0;
    // This matches the coefficient order produced by the corrected PolyFit.
    // The coefficients are stored as [c0, c1, c2, ...], so we evaluate c0*x^0 + c1*x^1 + c2*x^2 ...
    for (int i = 0; i < coeffs.rows; ++i) {
        result += coeffs.at<double>(i) * std::pow(x, i);
    }
    return result;
}

void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order) {
    CV_Assert(src_x.rows > 0 && src_y.rows > 0 && src_x.total() == src_y.total() && src_x.rows >= order + 1);
    cv::Mat A = cv::Mat::zeros(src_x.rows, order + 1, CV_64F);
    for (int i = 0; i < src_x.rows; i++) {
        for (int j = 0; j <= order; j++) {
            // It builds the matrix for the polynomial c0*x^0 + c1*x^1 + c2*x^2 ...
            A.at<double>(i, j) = pow(src_x.at<double>(i), (double)j);
        }
    }
    cv::solve(A, src_y, dst, cv::DECOMP_SVD);
}

double CalculateMean(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double CalculateQuantile(std::vector<double>& data, double percentile) {
    if (data.empty()) return 0.0;
    size_t n = static_cast<size_t>(data.size() * percentile);
    n = std::min(n, data.size() - 1);
    std::nth_element(data.begin(), data.begin() + n, data.end());
    return data[n];
}

double EvaluatePolynomialDerivative(const cv::Mat& coeffs, double x) {
    if (coeffs.empty()) {
        return 0.0;
    }
    double result = 0.0;
    int order = coeffs.rows - 1;
    // Derivative of a_n*x^n is n*a_n*x^(n-1)
    for (int i = 0; i < order; ++i) {
        double coeff = coeffs.at<double>(i);
        int power = order - i;
        result += static_cast<double>(power) * coeff * std::pow(x, power - 1);
    }
    return result;
}