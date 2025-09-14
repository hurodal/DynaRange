/**
 * @file core/Engine.cpp
 * @brief Implements the main orchestrator for the analysis workflow.
 */
#include "Engine.hpp"
#include "engine/Initialization.hpp"
#include "engine/Processing.hpp"
#include "engine/Reporting.hpp"

ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {
    // Phase 1: Preparation
    if (!InitializeAnalysis(opts, log_stream)) {
        return {}; // Return an empty ReportOutput on failure
    }
    
    // Phase 2: Processing of all files
    ProcessingResult results = ProcessFiles(opts, log_stream);

    // Phase 3: Generation of final reports
    return FinalizeAndReport(results, opts, log_stream);
}