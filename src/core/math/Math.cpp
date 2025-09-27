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
    int order = coeffs.rows - 1;
    for (int i = 0; i <= order; ++i) {
        result += coeffs.at<double>(i) * std::pow(x, order - i);
    }
    return result;
}

void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order) {
    CV_Assert(src_x.rows > 0 && src_y.rows > 0 && src_x.total() == src_y.total() && src_x.rows >= order + 1);
    cv::Mat A = cv::Mat::zeros(src_x.rows, order + 1, CV_64F);
    for (int i = 0; i < src_x.rows; ++i) {
        for (int j = 0; j <= order; ++j) {
            A.at<double>(i, j) = std::pow(src_x.at<double>(i), j);
        }
    }
    cv::Mat A_flipped;
    cv::flip(A, A_flipped, 1);
    cv::solve(A_flipped, src_y, dst, cv::DECOMP_SVD);
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