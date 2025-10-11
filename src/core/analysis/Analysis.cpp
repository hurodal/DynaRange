// File: src/core/analysis/Analysis.cpp
/**
 * @file src/core/analysis/Analysis.cpp
 * @brief Implements the main orchestration function for dynamic range analysis.
 */
#include "Analysis.hpp"
#include "CurveCalculator.hpp"
#include <opencv2/core.hpp>

std::pair<DynamicRangeResult, CurveData> CalculateResultsFromPatches(
    PatchAnalysisResult &patch_data, const ProgramOptions &opts,
    const std::string &filename, double camera_resolution_mpx,
    DataSource channel) {

  // Pass the source channel to the curve calculation function
  SnrCurve snr_curve = CurveCalculator::CalculateSnrCurve(patch_data, opts, camera_resolution_mpx, channel);
  
  DynamicRangeResult dr_result;
  dr_result.filename = filename;
  dr_result.channel = channel;
  
  dr_result.dr_values_ev = CurveCalculator::CalculateDynamicRange(snr_curve, opts.snr_thresholds_db);

  CurveData curve_data;
  curve_data.filename = filename;
  curve_data.channel = channel;
  curve_data.points = snr_curve.points; // Assign the new points vector
  curve_data.poly_coeffs = snr_curve.poly_coeffs.clone();
  curve_data.generated_command = opts.generated_command;

  return {dr_result, curve_data};
}