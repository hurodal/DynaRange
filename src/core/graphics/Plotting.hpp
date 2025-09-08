// Fichero: core/graphics/Plotting.hpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include "Drawing.hpp"

// --- FUNCIONES PÚBLICAS DE GENERACIÓN DE GRÁFICOS ---

// Genera un gráfico PNG para una única curva de SNR.
void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs,
    std::ostream& log_stream 
);

// Genera un gráfico PNG resumen con todas las curvas de SNR.
std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_dir,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    std::ostream& log_stream 
);