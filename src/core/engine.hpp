// File: core/Engine.hpp
#pragma once
#include "Arguments.hpp"
#include <ostream>
#include <optional>
#include <string>

// Accepts opts by reference (&) to allow modifications
std::optional<std::string> RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream);