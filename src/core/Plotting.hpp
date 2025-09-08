// Fichero: core/Plotting.hpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include "Analysis.hpp" 

// Dimensiones base del lienzo.
constexpr int PLOT_WIDTH = 1920;
constexpr int PLOT_HEIGHT = 1080;

// --- DECLARACIONES DE FUNCIONES DE GRÁFICOS ---
void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs,
    std::ostream& log_stream 
);

std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_dir,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    std::ostream& log_stream 
);