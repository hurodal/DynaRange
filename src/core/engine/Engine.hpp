// File: src/core/engine/Engine.hpp
/**
 * @file src/core/engine/Engine.hpp
 * @brief Declares the main orchestrator function for the dynamic range analysis.
 */
#pragma once

#include "../arguments/ArgumentsOptions.hpp"
#include "Reporting.hpp"
#include <ostream>
#include <atomic>

namespace DynaRange {
    // This declaration does not need to change as the new parameter is an internal detail.
    ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag);
}