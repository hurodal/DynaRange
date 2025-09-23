/**
 * @file src/core/analysis/CurveCalculator.cpp
 * @brief Implements the mathematical calculations for SNR curve and dynamic range.
 */
#include "CurveCalculator.hpp"
#include "../math/Math.hpp"
#include "../math/Math.hpp"
#include <cmath>

namespace CurveCalculator {

SnrCurve CalculateSnrCurve(PatchAnalysisResult& patch_data, const ProgramOptions& opts, double camera_resolution_mpx) {
    SnrCurve curve;

    // 1. Calculate linear SNR values first
    std::vector<double> snr_linear;
    snr_linear.reserve(patch_data.signal.size());
    for (size_t i = 0; i < patch_data.signal.size(); ++i) {
        if (patch_data.noise[i] > 0) {
            snr_linear.push_back(patch_data.signal[i] / patch_data.noise[i]);
        }
    }

    // 2. Apply normalization factor to SNR values (after calculation, before fitting)
    if (opts.dr_normalization_mpx > 0 && camera_resolution_mpx > 0) {
        double norm_factor = sqrt(camera_resolution_mpx / opts.dr_normalization_mpx);
        for (double& snr : snr_linear) {
            snr *= norm_factor;
        }
    }

    // 3. Convert final linear SNR to dB and calculate EV from ORIGINAL signal
    for (size_t i = 0; i < snr_linear.size(); ++i) {
        curve.snr_db.push_back(20 * log10(snr_linear[i]));
        curve.signal_ev.push_back(log2(patch_data.signal[i])); // ← ¡CORRECTO!
    }

    // 4. Fit polynomial to the final data
    //cv::Mat signal_mat_global(curve.signal_ev.size(), 1, CV_64F, curve.signal_ev.data());
    //cv::Mat snr_mat_global(curve.snr_db.size(), 1, CV_64F, curve.snr_db.data());
    //PolyFit(signal_mat_global, snr_mat_global, curve.poly_coeffs, opts.poly_order);

    // 4. Fit polynomial to the final data, swapping variables: EV = f(SNR_dB)
    cv::Mat signal_mat_global(curve.signal_ev.size(), 1, CV_64F, curve.signal_ev.data());
    cv::Mat snr_mat_global(curve.snr_db.size(), 1, CV_64F, curve.snr_db.data());
    // Invertimos las variables: SNR es ahora X, EV es ahora Y.
    PolyFit(snr_mat_global, signal_mat_global, curve.poly_coeffs, opts.poly_order);
    return curve;    
}

std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db) {
    std::map<double, double> dr_values_ev;
    if (snr_curve.signal_ev.empty()) {
        return dr_values_ev;
    }
    // El rango min/max de EV ya no es necesario para la llamada a FindIntersectionEV
    for (const double threshold_db : thresholds_db) {
        // Llama a la nueva versión simplificada de la función
        auto ev_opt = CalculateEVFromSNR(snr_curve.poly_coeffs, threshold_db);
        if (ev_opt) {
            dr_values_ev[threshold_db] = -(*ev_opt);
        }
    }
    return dr_values_ev;
}

} // namespace CurveCalculator