// File: src/core/engine/PatchAnalysisStrategy.hpp
/**
 * @file PatchAnalysisStrategy.hpp
 * @brief Declares the function for executing the two-pass patch analysis strategy.
 * @details This module encapsulates the complex logic of performing a strict
 * analysis pass, validating its results, and conditionally re-running a more
 * permissive pass to handle high-ISO "floating curves". It adheres to SRP
 * by separating this strategy from the main file processing orchestration.
 */
#pragma once

#include "../analysis/Analysis.hpp"
#include "../arguments/ArgumentsOptions.hpp"
#include "../setup/ChartProfile.hpp"
#include <ostream>

namespace DynaRange::Engine {

/**
 * @brief Executes a two-pass analysis strategy on a prepared single-channel image.
 * @details This function first analyzes the image with a strict SNR threshold. If it
 * detects that the resulting data points are all above the user's highest
 * requested SNR threshold (a "floating curve"), it discards the results and
 * re-analyzes the image with a much more permissive threshold to capture
 * deep shadow data.
 * @param prepared_image The single-channel image, already corrected for keystone and cropped.
 * @param channel The data source channel being analyzed (for logging).
 * @param chart The chart profile containing grid dimensions.
 * @param opts The program options.
 * @param log_stream The output stream for logging messages.
 * @param strict_min_snr_db The strict minimum SNR threshold for the first pass.
 * @param permissive_min_snr_db The permissive minimum SNR threshold for the second pass.
 * @param max_requested_threshold The highest SNR threshold requested by the user, used for validation.
 * @param create_overlay_image Flag to indicate if an overlay image should be generated.
 * @return A PatchAnalysisResult struct containing the signal, noise, and optional overlay image from the chosen pass.
 */
PatchAnalysisResult PerformTwoPassPatchAnalysis(
    const cv::Mat& prepared_image,
    DataSource channel,
    const ChartProfile& chart,
    const ProgramOptions& opts,
    std::ostream& log_stream,
    double strict_min_snr_db,
    double permissive_min_snr_db,
    double max_requested_threshold,
    bool create_overlay_image
);

} // namespace DynaRange::Engine