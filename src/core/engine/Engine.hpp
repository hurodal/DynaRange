// File: src/core/engine/Engine.hpp
/**
 * @file src/core/engine/Engine.hpp
 * @brief Declares the main orchestrator function for the dynamic range analysis.
 */
#pragma once

#include "../arguments/ProgramOptions.hpp"
#include "Reporting.hpp"
#include <ostream>

namespace DynaRange {

    ReportOutput RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream);

} // namespace DynaRange