// File: core/graphics/Plotting.hpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>

#include "Drawing.hpp"

void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs,
    const ProgramOptions& opts,
    std::ostream& log_stream 
);

std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_dir,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    const ProgramOptions& opts,
    std::ostream& log_stream 
);