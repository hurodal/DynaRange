// Fichero: core/Engine.hpp
#pragma once
#include "Arguments.hpp"
#include <ostream>
#include <optional>
#include <string>

// Acepta opts por referencia (&) para poder modificarla
std::optional<std::string> RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream);