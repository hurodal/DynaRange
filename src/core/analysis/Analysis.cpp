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

  SnrCurve snr_curve = CurveCalculator::CalculateSnrCurve(
      patch_data, opts, camera_resolution_mpx);
  
  DynamicRangeResult dr_result;
  dr_result.filename = filename;
  dr_result.channel = channel;
  // The responsibility of setting patch counts is moved to the caller (AnalyzeSingleRawFile).
  
  dr_result.dr_values_ev = CurveCalculator::CalculateDynamicRange(snr_curve, opts.snr_thresholds_db);

  CurveData curve_data;
  curve_data.filename = filename;
  curve_data.channel = channel;
  curve_data.signal_ev = snr_curve.signal_ev;
  curve_data.snr_db = snr_curve.snr_db;
  curve_data.poly_coeffs = snr_curve.poly_coeffs.clone();
  curve_data.generated_command = opts.generated_command;

  return {dr_result, curve_data};
}