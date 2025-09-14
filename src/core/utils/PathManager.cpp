/**
 * @file core/utils/PathManager.cpp
 * @brief Implements the PathManager utility class.
 */
#include "PathManager.hpp"
#include <sstream>
#include <algorithm>

PathManager::PathManager(const ProgramOptions& opts) {
    fs::path full_csv_path(opts.output_filename);
    m_output_directory = full_csv_path.parent_path();
    m_csv_filename = full_csv_path.filename();
}

fs::path PathManager::GetCsvOutputPath() const {
    return m_output_directory / m_csv_filename;
}

fs::path PathManager::GetIndividualPlotPath(const CurveData& curve) const {
    std::stringstream new_filename_ss;
    new_filename_ss << fs::path(curve.filename).stem().string();

    if (curve.iso_speed > 0) {
        new_filename_ss << "_ISO" << static_cast<int>(curve.iso_speed);
    }

    new_filename_ss << "_snr_plot";

    if (!curve.camera_model.empty()) {
        std::string safe_model = curve.camera_model;
        std::replace(safe_model.begin(), safe_model.end(), ' ', '_');
        new_filename_ss << "_" << safe_model;
    }

    new_filename_ss << ".png";
    
    return m_output_directory / new_filename_ss.str();
}

fs::path PathManager::GetSummaryPlotPath(const std::string& camera_name) const {
    std::string safe_camera_name = camera_name;
    std::replace(safe_camera_name.begin(), safe_camera_name.end(), ' ', '_');
    std::string filename = "DR_summary_plot_" + safe_camera_name + ".png";
    return m_output_directory / filename;
}