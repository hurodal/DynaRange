// File: src/core/Engine.cpp
/**
 * @file src/core/Engine.cpp
 * @brief Implements the main orchestrator for the analysis workflow.
 */
#include "Engine.hpp"
#include "Initialization.hpp"
#include "Processing.hpp"
#include "Reporting.hpp"
#include "Validation.hpp"

namespace DynaRange {

ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {
    // Phase 1: Preparation
    if (!InitializeAnalysis(opts, log_stream)) {
        return {}; // Return an empty ReportOutput on failure
    }
    
    // Phase 2: Processing of all files
    ProcessingResult results = ProcessFiles(opts, log_stream);

    // Phase 3: Validate SNR data before final reporting
    ValidateSnrResults(results, opts, log_stream);

    // Phase 4: Generation of final reports
    return FinalizeAndReport(results, opts, log_stream);
}

} // namespace DynaRange