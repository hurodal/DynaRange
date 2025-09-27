// File: src/core/analysis/CurveCalculator.cpp
/**
 * @file src/core/analysis/CurveCalculator.cpp
 * @brief Implements the mathematical calculations for SNR curve and dynamic range.
 */
#include "CurveCalculator.hpp"
#include "../math/Math.hpp"
#include <cmath>
#include <algorithm>
#include <optional>
#include <iostream>

namespace CurveCalculator {

// This function existed previously and has been modified.
SnrCurve CalculateSnrCurve(PatchAnalysisResult& patch_data, const ProgramOptions& opts, double camera_resolution_mpx) {
    SnrCurve curve;
    // By building the EV and SNR vectors together, we ensure each data point (EV, SNR_dB)
    // corresponds to the same physical patch from the test chart.
    for (size_t i = 0; i < patch_data.signal.size(); ++i) {
        double signal_val = patch_data.signal[i];
        double noise_val = patch_data.noise[i];

        // Process only valid patch data to avoid mathematical errors (e.g., log(0)).
        if (signal_val <= 0 || noise_val <= 0) {
            continue;
        }
        
        // 1. Calculate the linear Signal-to-Noise Ratio.
        double snr_linear = signal_val / noise_val;

        // 2. Apply normalization based on sensor resolution if specified.
        if (opts.dr_normalization_mpx > 0 && camera_resolution_mpx > 0) {
            double norm_factor = sqrt(camera_resolution_mpx / opts.dr_normalization_mpx);
            snr_linear *= norm_factor;
        }
        
        // 3. Convert to the plot's units (EV and dB) and add to the curve's data vectors.
        curve.signal_ev.push_back(log2(signal_val));
        curve.snr_db.push_back(20 * log10(snr_linear));
    }

    // Early exit if no valid curve data was generated after filtering.
    if (curve.signal_ev.empty() || curve.snr_db.empty()) {
        return curve;
    }

    // 4. Fit polynomial to the new EV = f(SNR_dB) model.
    // The roles of the variables are swapped here.
    cv::Mat signal_mat_global(curve.signal_ev.size(), 1, CV_64F, curve.signal_ev.data());
    cv::Mat snr_mat_global(curve.snr_db.size(), 1, CV_64F, curve.snr_db.data());
    PolyFit(snr_mat_global, signal_mat_global, curve.poly_coeffs, opts.poly_order);
    
    return curve;
}

// This function existed previously and has been modified.
std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_values_ev;
    if (snr_curve.signal_ev.empty()) {
        return dr_values_ev;
    }

    // The theoretical saturation point is at EV=0.
    // The Dynamic Range is the distance from this point to the EV where the curve
    // crosses the SNR threshold.
    // DR = EV_saturation - EV_threshold = 0 - EV_threshold = -EV_threshold.
    
    for (const double threshold_db : thresholds_db) {
        // With the new model EV = f(SNR_dB), we can directly evaluate the polynomial
        // at the desired threshold to get the corresponding EV.
        double ev_at_threshold = EvaluatePolynomial(snr_curve.poly_coeffs, threshold_db);
        
        // The dynamic range calculation remains the same.
        dr_values_ev[threshold_db] = -ev_at_threshold;
    }
    return dr_values_ev;
}

} // namespace CurveCalculator