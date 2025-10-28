// File: src/core/artifacts/plot/PlotWriter.hpp
/**
 * @file src/core/artifacts/plot/PlotWriter.hpp
 * @brief Declares functions for generating and saving plot image artifacts.
 * @details This module encapsulates the content drawing (Cairo) and saving logic for plots,
 * delegating only naming and bounds calculation to other modules.
 */
#pragma once

#include "../../analysis/Analysis.hpp"
#include "../../engine/Reporting.hpp"
#include "../../utils/OutputNamingContext.hpp"
#include "../../utils/PathManager.hpp"
#include <filesystem>
#include <optional>
#include <ostream>
#include <vector>
#include <map>

namespace fs = std::filesystem;

namespace ArtifactFactory::Plot {

/**
 * @brief Creates and saves the summary plot image.
 * @param curves The aggregated curve data for all files.
 * @param results The aggregated dynamic range results (for labels).
 * @param ctx The context for generating filename and title.
 * @param reporting_params Parameters controlling plot rendering details.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved plot file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateSummaryPlot(
    const std::vector<CurveData>& curves,
    const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx,
    const ReportingParameters& reporting_params,
    const PathManager& paths,
    std::ostream& log_stream);

/**
 * @brief Creates and saves an individual plot image for a single input file/ISO.
 * @param curves_for_file The curve data specific to this file.
 * @param results_for_file The dynamic range results specific to this file.
 * @param ctx The context for generating filename and title (must include ISO).
 * @param reporting_params Parameters controlling plot rendering details.
 * @param global_bounds Pre-calculated global axis boundaries for consistent scaling.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved plot file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateIndividualPlot(
    const std::vector<CurveData>& curves_for_file,
    const std::vector<DynamicRangeResult>& results_for_file,
    const OutputNamingContext& ctx,
    const ReportingParameters& reporting_params,
    const std::map<std::string, double>& global_bounds,
    const PathManager& paths,
    std::ostream& log_stream);

} // namespace ArtifactFactory::Plot