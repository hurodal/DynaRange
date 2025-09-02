// core/engine.hpp
#pragma once
#include "arguments.hpp"
#include <ostream>

// The main engine function.
// Takes the configuration and an output stream for logs.
// Returns true on success, false on failure.
bool run_dynamic_range_analysis(const ProgramOptions& opts, std::ostream& log_stream);