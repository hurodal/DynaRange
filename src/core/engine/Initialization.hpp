// File: core/engine/Initialization.hpp
#pragma once
#include "../Arguments.hpp"
#include <ostream>

// Prepares the analysis: processes dark/sat frames, prints config, and sorts files.
bool InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream);