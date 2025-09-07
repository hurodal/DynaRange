// Fichero: core/Plotting.hpp
#pragma once

#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include "Analysis.hpp" // Necesita CurveData

// Dimensiones base del lienzo. Cambia estos valores y el gráfico se reescalará automáticamente.
constexpr int PLOT_WIDTH = 3840;
constexpr int PLOT_HEIGHT = 2160;

// --- DECLARACIONES DE FUNCIONES DE GRÁFICOS ---
void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs
);

void GenerateSummaryPlot(
    const std::string& output_filename,
    const std::vector<CurveData>& all_curves
);