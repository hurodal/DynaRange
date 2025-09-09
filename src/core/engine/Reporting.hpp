// File: core/engine/Reporting.hpp
#pragma once
#include "Processing.hpp" // For ProcessingResult
#include <string>

// Generates the final reports: summary plot, table, and CSV.
std::optional<std::string> FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream);