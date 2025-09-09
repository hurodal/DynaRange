// File: core/engine/Processing.hpp
#pragma once
#include "../Analysis.hpp"
#include <vector>

// Structure to return the results of a single file
struct SingleFileResult {
    DynamicRangeResult dr_result;
    CurveData curve_data;
};

// Structure to return the processing results of all files
struct ProcessingResult {
    std::vector<DynamicRangeResult> dr_results;
    std::vector<CurveData> curve_data;
};

// Processes the list of RAW files and returns the analyzed data.
ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream);