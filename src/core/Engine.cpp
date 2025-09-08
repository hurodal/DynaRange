// Fichero: core/Engine.cpp
#include "Engine.hpp"
#include "engine/Initialization.hpp"
#include "engine/Processing.hpp"
#include "engine/Reporting.hpp"

std::optional<std::string> RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {
    // Fase 1: Preparación
    if (!InitializeAnalysis(opts, log_stream)) {
        return std::nullopt;
    }
    
    // Fase 2: Procesamiento de todos los ficheros
    ProcessingResult results = ProcessFiles(opts, log_stream);

    // Fase 3: Generación de informes finales
    return FinalizeAndReport(results, opts, log_stream);
}