// Fichero: core/Plotting.hpp
#pragma once

#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include "Analysis.hpp"

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