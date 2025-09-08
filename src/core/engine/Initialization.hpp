// Fichero: core/engine/Initialization.hpp
#pragma once
#include "../Arguments.hpp"
#include <ostream>

// Prepara el análisis: procesa dark/sat frames, imprime config y ordena ficheros.
bool InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream);