// File: src/core/analysis/CurveCalculator.cpp

#include "CurveCalculator.hpp"
#include "../math/Math.hpp"
#include <cmath>
#include <algorithm>
#include <vector>
#include <libintl.h>

#define _(string) gettext(string)

namespace CurveCalculator {

SnrCurve CalculateSnrCurve(const PatchAnalysisResult& patch_data, const ProgramOptions& opts, double camera_resolution_mpx, DataSource source_channel) {
    if (patch_data.signal.empty()) {
        return {};
    }

    double norm_factor = 1.0;
    if (opts.dr_normalization_mpx > 0 && camera_resolution_mpx > 0) {
        norm_factor = std::sqrt(camera_resolution_mpx / opts.dr_normalization_mpx);
    }

    std::vector<PointData> points;
    points.reserve(patch_data.signal.size());

    for (size_t i = 0; i < patch_data.signal.size(); ++i) {
        double S = patch_data.signal[i];
        double N = patch_data.noise[i] / norm_factor;

        if (S <= 0.0 || N <= 0.0) continue;

        PointData point;
        point.ev = std::log2(S);
        point.snr_db = 20.0 * std::log10(S / N);
        point.channel = (i < patch_data.channels.size()) ? patch_data.channels[i] : source_channel;
        
        points.push_back(point);
    }
    
    if (points.size() < static_cast<size_t>(opts.poly_order + 1)) {
        return {points, cv::Mat()};
    }

    std::vector<double> ev_values, snr_db_values;
    ev_values.reserve(points.size());
    snr_db_values.reserve(points.size());
    for (const auto& p : points) {
        ev_values.push_back(p.ev);
        snr_db_values.push_back(p.snr_db);
    }

    cv::Mat ev_mat(ev_values.size(), 1, CV_64F, ev_values.data());
    cv::Mat snr_db_mat(snr_db_values.size(), 1, CV_64F, snr_db_values.data());
    cv::Mat poly_coeffs;
    PolyFit(ev_mat, snr_db_mat, poly_coeffs, opts.poly_order);
    
    return {points, poly_coeffs};
}

std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_map;
    const auto& poly_coeffs = snr_curve.poly_coeffs;
    if (snr_curve.points.empty() || poly_coeffs.empty()) {
        return dr_map;
    }
    
    for (double threshold : thresholds_db) {
        double ev_at_threshold = 0.0;
        if (poly_coeffs.rows == 4) { // Cubic (order 3)
            double best_ev = 0.0;
            double min_diff = 1e9;
            for (double ev = -30.0; ev < 5.0; ev += 0.0001) {
                double snr_est = EvaluatePolynomial(poly_coeffs, ev);
                double diff = std::abs(snr_est - threshold);
                if (diff < min_diff) {
                    min_diff = diff;
                    best_ev = ev;
                }
            }
            ev_at_threshold = best_ev;
        } else if (poly_coeffs.rows == 3) { // Quadratic (order 2)
            double c2 = poly_coeffs.at<double>(0);
            double c1 = poly_coeffs.at<double>(1);
            double c0 = poly_coeffs.at<double>(2);
            double discriminant = c1 * c1 - 4 * c2 * (c0 - threshold);
            if (discriminant >= 0) {
                double root1 = (-c1 + std::sqrt(discriminant)) / (2 * c2);
                double root2 = (-c1 - std::sqrt(discriminant)) / (2 * c2);
                ev_at_threshold = std::min(root1, root2);
            }
        }
        
        dr_map[threshold] = -ev_at_threshold;
    }

    return dr_map;
}

} // namespace CurveCalculator