// Fichero: core/Engine.hpp
#pragma once
#include "Arguments.hpp"
#include <ostream>

// Declaración de la función principal del motor, ahora en PascalCase.
bool RunDynamicRangeAnalysis(const ProgramOptions& opts, std::ostream& log_stream);