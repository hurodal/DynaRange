// File: src/core/analysis/CurveCalculator.cpp
/**
 * @file src/core/analysis/CurveCalculator.cpp
 * @brief Implements the mathematical calculations for SNR curve and dynamic range.
 */
#include "CurveCalculator.hpp"
#include "../math/Math.hpp"
#include <cmath>
#include <algorithm>

namespace CurveCalculator {

SnrCurve CalculateSnrCurve(PatchAnalysisResult& patch_data, const ProgramOptions& opts, double camera_resolution_mpx) {
    SnrCurve curve;
    for (size_t i = 0; i < patch_data.signal.size(); ++i) {
        double signal_val = patch_data.signal[i];
        double noise_val = patch_data.noise[i];

        if (signal_val <= 0 || noise_val <= 0) {
            continue;
        }
        
        double snr_linear = signal_val / noise_val;

        if (opts.dr_normalization_mpx > 0 && camera_resolution_mpx > 0) {
            double norm_factor = sqrt(camera_resolution_mpx / opts.dr_normalization_mpx);
            snr_linear *= norm_factor;
        }
        
        curve.signal_ev.push_back(log2(signal_val));
        curve.snr_db.push_back(20 * log10(snr_linear));
    }

    if (curve.signal_ev.empty() || curve.snr_db.empty()) {
        return curve;
    }

    // Fit polynomial to the EV = f(SNR_dB) model.
    // This aligns with the reference R script and the expectations of the DrawCurve function.
    // The independent variable (X) is now SNR in dB, and the dependent variable (Y) is Exposure Value (EV).
    cv::Mat snr_mat_global(curve.snr_db.size(), 1, CV_64F, curve.snr_db.data());
    cv::Mat signal_mat_global(curve.signal_ev.size(), 1, CV_64F, curve.signal_ev.data());
    PolyFit(snr_mat_global, signal_mat_global, curve.poly_coeffs, opts.poly_order);

    return curve;
}

std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_values_ev;
    if (snr_curve.signal_ev.empty()) {
        return dr_values_ev;
    }

    // Encuentra el rango de EV de los datos originales para limitar la búsqueda.
    auto min_max_ev = std::minmax_element(snr_curve.signal_ev.begin(), snr_curve.signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;

    for (const double threshold_db : thresholds_db) {
        // With the new EV = f(SNR_dB) model, we can directly evaluate the polynomial.
        // The dynamic range is the distance from saturation (EV=0) to the EV where the curve
        // crosses the SNR threshold. DR = EV_saturation - EV_threshold = 0 - EV_threshold = -EV_threshold.
        double ev_at_threshold = EvaluatePolynomial(snr_curve.poly_coeffs, threshold_db);

        // El rango dinámico es la distancia desde la saturación (EV=0) al umbral.
        dr_values_ev[threshold_db] = -ev_at_threshold;
    }
    return dr_values_ev;
}

} // namespace CurveCalculator