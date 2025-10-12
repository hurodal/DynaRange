// File: src/core/engine/Engine.cpp
/**
 * @file src/core/Engine.cpp
 * @brief Implements the main orchestrator for the analysis workflow.
 */
#include "Engine.hpp"
#include "Initialization.hpp"
#include "processing/Processing.hpp"
#include "Reporting.hpp"
#include "Validation.hpp"
#include <atomic>
#include <ostream>
#include <libintl.h>

#define _(string) gettext(string)

namespace DynaRange {

/**
 * @brief Orchestrates the entire dynamic range analysis workflow from start to finish.
 * @param opts A reference to the program options, which may be updated during initialization.
 * @param log_stream The output stream for logging all messages.
 * @param cancel_flag An atomic boolean flag that can be set from another thread to request cancellation.
 * @return A ReportOutput struct containing paths to the generated plots, or an empty struct on failure or cancellation.
 */
ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    // Phase 1: Preparation
    if (!InitializeAnalysis(opts, log_stream)) {
        return {}; // Return an empty ReportOutput on failure
    }
    
    // Phase 2: Processing of all files
    ProcessingResult results = ProcessFiles(opts, log_stream, cancel_flag);
    
    // Check if the user cancelled the operation during processing
    if (cancel_flag) {
        log_stream << "\n" << _("[INFO] Analysis cancelled by user.") << std::endl;
        return {};
    }

    // Phase 3: Validate SNR data before final reporting
    ValidateSnrResults(results, opts, log_stream);

    // Phase 4: Generation of final reports
    ReportOutput report = FinalizeAndReport(results, opts, log_stream);

    // Populate the new dr_results member with the sorted results.
    // for combining the report artifacts with the numerical results
    // Needed when click event on grid results.csv and show graphic at GUI
    report.dr_results = results.dr_results;
    report.curve_data = results.curve_data;

    return report;
}

} // namespace DynaRange