// File: src/core/graphics/PlotDataGenerator.hpp
/**
 * @file PlotDataGenerator.hpp
 * @brief Declares the function for generating curve points for plotting.
 * @details This module adheres to SRP by separating the logic of calculating
 * plot points from the logic of drawing them.
 */
#pragma once

#include "../analysis/Analysis.hpp" // For CurveData
#include <vector>
#include <utility>

namespace PlotDataGenerator {

/**
 * @brief Generates the plottable points for a fitted SNR curve.
 * @details Based on the model selected in Constants.hpp, this function calculates
 * a series of (EV, SNR_dB) coordinate pairs that represent the fitted curve.
 * @param curve The CurveData containing the raw data and polynomial coefficients.
 * @return A vector of pairs, where each pair is an (EV, SNR_dB) point on the curve.
 */
std::vector<std::pair<double, double>> GenerateCurvePoints(const CurveData& curve);

} // namespace PlotDataGenerator