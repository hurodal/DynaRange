// File: src/core/graphics/PlotDataGenerator.cpp
#include "PlotDataGenerator.hpp"
#include "../math/Math.hpp"
#include <algorithm> // For std::minmax_element

namespace PlotDataGenerator {

// This function generates points for the EV = f(SNR_dB) model.
std::vector<std::pair<double, double>> GeneratePointsForEVEqualsFSNR(const CurveData& curve) {
    std::vector<std::pair<double, double>> points;
    if (curve.points.empty() || curve.poly_coeffs.empty()) {
        return points;
    }

    // Find the min/max range of the SNR data, which is our independent variable 'x'.
    auto min_max_it = std::minmax_element(curve.points.begin(), curve.points.end(),
        [](const PointData& a, const PointData& b) {
            return a.snr_db < b.snr_db;
        });
    double min_snr_data = min_max_it.first->snr_db;
    double max_snr_data = min_max_it.second->snr_db;

    const int NUM_POINTS = 200; // Increased points for a smoother curve
    points.reserve(NUM_POINTS + 1);

    // Iterate over the SNR range to draw the curve EV = f(SNR_dB).
    for (int i = 0; i <= NUM_POINTS; ++i) {
        double snr_step = min_snr_data + (i * (max_snr_data - min_snr_data) / NUM_POINTS);
        
        // Evaluate the polynomial P(SNR_dB) to get the corresponding EV.
        double ev_at_snr = EvaluatePolynomial(curve.poly_coeffs, snr_step);

        // Store the pair {EV, SNR_dB} for plotting.
        points.emplace_back(ev_at_snr, snr_step);
    }
    return points;
}


std::vector<std::pair<double, double>> GenerateCurvePoints(const CurveData& curve) {
    // We are now committed to the EV = f(SNR_dB) model, so we call its specific generator.
    return GeneratePointsForEVEqualsFSNR(curve);
}

} // namespace PlotDataGenerator