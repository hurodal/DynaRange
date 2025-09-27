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

/**
 * @brief Calculates the SNR curve from patch signal and noise data.
 * @param patch_data The result of the patch analysis.
 * @param opts The program options (for normalization and polynomial order).
 * @param camera_resolution_mpx The camera's actual resolution in megapixels.
 * @return An SnrCurve struct containing the calculated curve data.
 */
SnrCurve CalculateSnrCurve(PatchAnalysisResult& patch_data, const ProgramOptions& opts, double camera_resolution_mpx) {
    SnrCurve curve;

    // CORRECTED LOGIC: Process signal and noise in a single loop.
    // This prevents the data mismatch bug that caused incorrect curve fitting.
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

    // DEBUG POINT 3: Check final SNR range before fitting
    auto min_max_snr = std::minmax_element(curve.snr_db.begin(), curve.snr_db.end());
    std::cout << "DEBUG ACTUAL SNR - Valid Points: " << curve.snr_db.size() << "\n";
    std::cout << "DEBUG ACTUAL SNR - Min SNR (dB): " << *min_max_snr.first << "\n";
    std::cout << "DEBUG ACTUAL SNR - Max SNR (dB): " << *min_max_snr.second << "\n\n";

    // 4. Fit polynomial to the final SNR_dB = f(EV) data.
    cv::Mat signal_mat_global(curve.signal_ev.size(), 1, CV_64F, curve.signal_ev.data());
    cv::Mat snr_mat_global(curve.snr_db.size(), 1, CV_64F, curve.snr_db.data());
    PolyFit(signal_mat_global, snr_mat_global, curve.poly_coeffs, opts.poly_order);
    
    return curve;
}

/**
 * @brief Calculates dynamic range values for a set of SNR thresholds.
 * @param snr_curve The calculated SNR curve.
 * @param thresholds_db The vector of SNR thresholds in dB.
 * @return A map of threshold to calculated dynamic range in EV.
 */
std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_values_ev;
    if (snr_curve.signal_ev.empty()) {
        return dr_values_ev;
    }

    // The theoretical saturation point is at EV=0, which corresponds to a normalized signal of 1.0.
    // The Dynamic Range is the distance from the EV where the curve crosses the SNR threshold
    // to this saturation point.
    // Therefore, DR = EV_saturation - EV_threshold = 0 - EV_threshold = -EV_threshold.
    
    // Get the min/max EV range of the data to bound the intersection search.
    auto min_max_ev = std::minmax_element(snr_curve.signal_ev.begin(), snr_curve.signal_ev.end());
    double min_ev = *min_max_ev.first;
    double max_ev = *min_max_ev.second;

    for (const double threshold_db : thresholds_db) {
        auto ev_opt = FindIntersectionEV(snr_curve.poly_coeffs, threshold_db, min_ev, max_ev);
        if (ev_opt) {
            // CORRECTED LOGIC: Reverted to the original, correct formula.
            dr_values_ev[threshold_db] = -(*ev_opt);
        }
    }
    return dr_values_ev;
}

} // namespace CurveCalculator