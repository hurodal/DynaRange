// File: src/core/analysis/CurveCalculator.hpp
/**
 * @file src/core/analysis/CurveCalculator.hpp
 * @brief Declares functions for calculating SNR curves and dynamic range from patch data.
 */
#pragma once
#include "Analysis.hpp"
#include "../arguments/ArgumentsOptions.hpp"

namespace CurveCalculator {
/**
 * @brief Calculates the SNR curve from patch signal and noise data.
 * @param patch_data The result of the patch analysis.
 * @param params The consolidated analysis parameters (for normalization and polynomial order).
 * @param source_channel The primary channel for this analysis.
 * @return An SnrCurve struct containing the calculated curve data.
 */
SnrCurve CalculateSnrCurve(const PatchAnalysisResult& patch_data, const AnalysisParameters& params, DataSource source_channel);

/**
 * @brief Calculates dynamic range values for a set of SNR thresholds.
 * @param snr_curve The calculated SNR curve.
 * @param thresholds_db The vector of SNR thresholds in dB.
 * @return A map of threshold to calculated dynamic range in EV.
 */
std::map<double, double> CalculateDynamicRange(const SnrCurve& snr_curve, const std::vector<double>& thresholds_db);
} // namespace CurveCalculator