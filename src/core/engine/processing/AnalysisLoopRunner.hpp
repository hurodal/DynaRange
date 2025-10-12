// File: src/core/engine/processing/AnalysisLoopRunner.hpp
/**
 * @file AnalysisLoopRunner.hpp
 * @brief Declares a component for running the main analysis loop over a set of files.
 * @details This module adheres to SRP by encapsulating the entire loop execution,
 * including keystone optimization strategy and result aggregation, separating it
 * from the high-level orchestration in ProcessFiles.
 */
#pragma once

#include "Processing.hpp"
#include "../../io/raw/RawFile.hpp"
#include "../../setup/ChartProfile.hpp"
#include "../../arguments/ArgumentsOptions.hpp"
#include <vector>
#include <string>
#include <ostream>
#include <atomic>

namespace DynaRange::Engine::Processing {

/**
 * @class AnalysisLoopRunner
 * @brief Executes the analysis loop over a list of RAW files.
 */
class AnalysisLoopRunner {
public:
    /**
     * @brief Constructs an AnalysisLoopRunner with the required context.
     * @param raw_files The vector of loaded RawFile objects to process.
     * @param opts The program options.
     * @param chart The geometric profile of the test chart.
     * @param camera_model_name The detected camera model name.
     * @param log_stream The output stream for logging.
     * @param cancel_flag The atomic flag for cancellation.
     */
    AnalysisLoopRunner(
        const std::vector<RawFile>& raw_files,
        const ProgramOptions& opts,
        const ChartProfile& chart,
        const std::string& camera_model_name,
        std::ostream& log_stream,
        const std::atomic<bool>& cancel_flag
    );

    /**
     * @brief Runs the analysis loop.
     * @return A ProcessingResult struct containing the aggregated results.
     */
    ProcessingResult Run();

private:
    const std::vector<RawFile>& m_raw_files;
    const ProgramOptions& m_opts;
    const ChartProfile& m_chart;
    const std::string& m_camera_model_name;
    std::ostream& m_log_stream;
    const std::atomic<bool>& m_cancel_flag;
};

} // namespace DynaRange::Engine::Processing