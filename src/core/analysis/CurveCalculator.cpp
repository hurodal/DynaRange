// File: src/core/analysis/CurveCalculator.cpp

#include "CurveCalculator.hpp"
#include "../math/Math.hpp"
#include <cmath>
#include <algorithm>
#include <vector>
#include <libintl.h>

#define _(string) gettext(string)

namespace CurveCalculator {

SnrCurve CalculateSnrCurve(const PatchAnalysisResult& patch_data, const ProgramOptions& opts, double camera_resolution_mpx) {
    if (patch_data.signal.empty()) {
        return {};
    }

    double norm_factor = 1.0;
    if (opts.dr_normalization_mpx > 0 && camera_resolution_mpx > 0) {
        norm_factor = std::sqrt(camera_resolution_mpx / opts.dr_normalization_mpx);
    }

    std::vector<double> ev_values;
    std::vector<double> snr_db_values;
    ev_values.reserve(patch_data.signal.size());
    snr_db_values.reserve(patch_data.signal.size());

    for (size_t i = 0; i < patch_data.signal.size(); ++i) {
        double S = patch_data.signal[i];
        double N = patch_data.noise[i] / norm_factor;

        if (S <= 0.0 || N <= 0.0)
            continue;
        
        // Use direct log2(S) to match the old version's EV calculation.
        double ev = std::log2(S);
        double snr_db = 20.0 * std::log10(S / N);

        ev_values.push_back(ev);
        snr_db_values.push_back(snr_db);
    }

    if (ev_values.size() < static_cast<size_t>(opts.poly_order + 1)) {
        return {ev_values, snr_db_values, cv::Mat()};
    }

    cv::Mat ev_mat(ev_values.size(), 1, CV_64F, ev_values.data());
    cv::Mat snr_db_mat(snr_db_values.size(), 1, CV_64F, snr_db_values.data());
    cv::Mat poly_coeffs;
    PolyFit(ev_mat, snr_db_mat, poly_coeffs, opts.poly_order);

    return {ev_values, snr_db_values, poly_coeffs};
}

std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_map;
    const auto& poly_coeffs = snr_curve.poly_coeffs;
    if (snr_curve.signal_ev.empty() || poly_coeffs.empty()) {
        return dr_map;
    }
    
    for (double threshold : thresholds_db) {
        double ev_at_threshold = 0.0;
        
        // Use the correct solver based on the polynomial order.
        if (poly_coeffs.rows == 4) { // Cubic (order 3)
            // Use a simple numerical root-finding loop, like the old version.
            double best_ev = 0.0;
            double min_diff = 1e9;
            // Iterate over a plausible EV range to find the intersection.
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