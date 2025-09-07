// Fichero: core/Engine.hpp
#pragma once
#include "Arguments.hpp"
#include <ostream>
#include <optional>
#include <string>


//  La función ahora devuelve un opcional<string> 
std::optional<std::string> RunDynamicRangeAnalysis(const ProgramOptions& opts, std::ostream& log_stream);