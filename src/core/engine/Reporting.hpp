// Fichero: core/engine/Reporting.hpp
#pragma once
#include "Processing.hpp" // Para ProcessingResult
#include <string>

// Genera los informes finales: gráfico resumen, tabla y CSV.
std::optional<std::string> FinalizeAndReport(
    const ProcessingResult& results,
    const ProgramOptions& opts,
    std::ostream& log_stream);