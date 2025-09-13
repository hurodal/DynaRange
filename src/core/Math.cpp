/**
 * @file core/Math.cpp
 * @brief Implements mathematical and statistical utility functions.
 */
#include "Math.hpp"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <numeric>
#include <algorithm>

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

std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev) {
    if (coeffs.empty()) {
        return std::nullopt;
    }
    int order = coeffs.rows - 1;

    // --- Case for Order 2 Polynomial (Quadratic) ---
    if (order == 2) {
        double c2 = coeffs.at<double>(0), c1 = coeffs.at<double>(1), c0 = coeffs.at<double>(2);
        double a = c2, b = c1, c = c0 - target_snr_db;
        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return std::nullopt;
        double sqrt_d = sqrt(discriminant);
        double ev1 = (-b + sqrt_d) / (2 * a);
        double ev2 = (-b - sqrt_d) / (2 * a);
        if (ev1 >= min_ev && ev1 <= max_ev) return ev1;
        if (ev2 >= min_ev && ev2 <= max_ev) return ev2;
        return std::nullopt;
    }
    
    // --- Case for Order 3 Polynomial (Cubic) using Newton-Raphson ---
    if (order == 3) {
        double c3 = coeffs.at<double>(0);
        double c2 = coeffs.at<double>(1);
        double c1 = coeffs.at<double>(2);
        double c0 = coeffs.at<double>(3) - target_snr_db;

        auto f = [&](double ev) { return c3 * pow(ev, 3) + c2 * pow(ev, 2) + c1 * ev + c0; };
        auto df = [&](double ev) { return 3 * c3 * pow(ev, 2) + 2 * c2 * ev + c1; };

        double ev_guess = (min_ev + max_ev) / 2.0;

        for (int i = 0; i < 10; ++i) {
            double f_val = f(ev_guess);
            double df_val = df(ev_guess);
            if (std::abs(df_val) < 1e-7) break;
            double next_ev = ev_guess - f_val / df_val;
            if (std::abs(next_ev - ev_guess) < 1e-7) {
                ev_guess = next_ev;
                break;
            }
            ev_guess = next_ev;
        }

        if (ev_guess >= min_ev && ev_guess <= max_ev) {
            return ev_guess;
        }
        return std::nullopt;
    }
    
    return std::nullopt;
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