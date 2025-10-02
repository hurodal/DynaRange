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
    const std::string &filename, double camera_resolution_mpx) {

  // Pass all necessary info to the curve calculation function
  SnrCurve snr_curve = CurveCalculator::CalculateSnrCurve(
      patch_data, opts, camera_resolution_mpx);

  DynamicRangeResult dr_result;
  dr_result.filename = filename;
  dr_result.patches_used = (int)patch_data.signal.size();

  // The call is updated to match the new function signature.
  dr_result.dr_values_ev = CurveCalculator::CalculateDynamicRange(snr_curve, opts.snr_thresholds_db);

  CurveData curve_data = {filename,
                          "", // plot_label (filled in Processing.cpp)
                          "", // camera_model (filled in Processing.cpp)
                          snr_curve.signal_ev,
                          snr_curve.snr_db,
                          snr_curve.poly_coeffs.clone(),
                          {}, // curve_points (initially empty, generated before plotting)
                          opts.generated_command};

  return {dr_result, curve_data};
}
