// Fichero: core/engine/Processing.hpp
#pragma once
#include "../Analysis.hpp"
#include <vector>

// Estructura para devolver los resultados de un único fichero
struct SingleFileResult {
    DynamicRangeResult dr_result;
    CurveData curve_data;
};

// Estructura para devolver los resultados del procesamiento de todos los ficheros
struct ProcessingResult {
    std::vector<DynamicRangeResult> dr_results;
    std::vector<CurveData> curve_data;
};

// Procesa la lista de ficheros RAW y devuelve los datos analizados.
ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream);