// File: src/core/analysis/CurveCalculator.cpp
/**
 * @file src/core/analysis/CurveCalculator.cpp
 * @brief Implements the SNR curve and dynamic range calculation logic.
 */
#include "CurveCalculator.hpp"
#include "../engine/processing/Processing.hpp"
#include "../math/Math.hpp"
#include <cmath>
#include <vector>
#include <libintl.h>

#define _(string) gettext(string)

namespace CurveCalculator {

SnrCurve CalculateSnrCurve(const PatchAnalysisResult& patch_data, const AnalysisParameters& params, DataSource source_channel) {
    if (patch_data.signal.empty()) {
        return {};
    }

    // Normalization factor is calculated once.
    double norm_factor = 1.0;
    if (params.dr_normalization_mpx > 0 && params.sensor_resolution_mpx > 0) {
        norm_factor = std::sqrt(params.sensor_resolution_mpx / params.dr_normalization_mpx);
    }

    std::vector<PointData> points;
    points.reserve(patch_data.signal.size());

    std::vector<double> ev_values;
    std::vector<double> snr_db_values;
    ev_values.reserve(patch_data.signal.size());
    snr_db_values.reserve(patch_data.signal.size());

    for (size_t i = 0; i < patch_data.signal.size(); ++i) {
        double S = patch_data.signal[i];
        double N = patch_data.noise[i];

        if (S <= 0.0 || N <= 0.0) continue;
        
        // Calculate SNR linear, apply normalization, and then convert to dB.
        double snr_linear = (S / N) * norm_factor;

        PointData point;
        // EV is the absolute log2 of the signal.
        point.ev = std::log2(S);
        point.snr_db = 20.0 * std::log10(snr_linear);
        point.channel = (i < patch_data.channels.size()) ? patch_data.channels[i] : source_channel;
        
        points.push_back(point);
        ev_values.push_back(point.ev);
        snr_db_values.push_back(point.snr_db);
    }
    
    if (points.size() < static_cast<size_t>(params.poly_order + 1)) {
        return {points, cv::Mat()};
    }

    // The model is EV = f(SNR_dB), so PolyFit's arguments are swapped.
    // PolyFit(x, y, coeffs) -> PolyFit(snr_db, ev, coeffs)
    cv::Mat ev_mat(ev_values.size(), 1, CV_64F, ev_values.data());
    cv::Mat snr_db_mat(snr_db_values.size(), 1, CV_64F, snr_db_values.data());
    cv::Mat poly_coeffs;
    PolyFit(snr_db_mat, ev_mat, poly_coeffs, params.poly_order);
    
    return {points, poly_coeffs};
}

std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_map;
    const auto& poly_coeffs = snr_curve.poly_coeffs;
    if (snr_curve.points.empty() || poly_coeffs.empty()) {
        return dr_map;
    }
    
    // With the EV = f(SNR_dB) model, the calculation is a direct evaluation.
    // The complex solvers are no longer needed.
    for (double threshold : thresholds_db) {
        // We evaluate the polynomial at the SNR threshold to get the corresponding EV.
        double ev_at_threshold = EvaluatePolynomial(poly_coeffs, threshold);
        // The DR formula remains DR = -EV_threshold.
        dr_map[threshold] = -ev_at_threshold;
    }

    return dr_map;
}

} // namespace CurveCalculator