// File: src/core/graphics/PlotDataGenerator.cpp
/**
 * @file PlotDataGenerator.cpp
 * @brief Implements the plot data generation logic.
 */
#include "PlotDataGenerator.hpp"
#include "../Constants.hpp"
#include "../math/Math.hpp"
#include <algorithm> // For std::minmax_element

namespace PlotDataGenerator {

/**
 * @brief (Internal) Generates points using the SNR_dB = f(EV) model.
 * @param curve The curve data.
 * @return A vector of (EV, SNR_dB) points.
 */
std::vector<std::pair<double, double>> GeneratePointsForSnrEqualsFEV(const CurveData& curve) {
    std::vector<std::pair<double, double>> points;
    if (curve.signal_ev.empty() || curve.poly_coeffs.empty()) {
        return points;
    }

    auto min_max_ev = std::minmax_element(curve.signal_ev.begin(), curve.signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;

    const int NUM_POINTS = 100;
    points.reserve(NUM_POINTS + 1);

    for (int i = 0; i <= NUM_POINTS; ++i) {
        double ev_step = min_ev_data + (i * (max_ev_data - min_ev_data) / NUM_POINTS);
        double snr_at_ev = EvaluatePolynomial(curve.poly_coeffs, ev_step);
        points.emplace_back(ev_step, snr_at_ev);
    }
    return points;
}


std::vector<std::pair<double, double>> GenerateCurvePoints(const CurveData& curve) {
    switch (DynaRange::Constants::PLOTTING_MODEL) {
        case DynaRange::Constants::PlottingModel::SNR_equals_f_EV:
            return GeneratePointsForSnrEqualsFEV(curve);

        case DynaRange::Constants::PlottingModel::EV_equals_f_SNR:
            // Future implementation for the R model would go here.
            // For now, it can fall through or return an empty vector.
            return {};
    }
    return {}; // Should not be reached
}

} // namespace PlotDataGenerator