/**
 * @file core/utils/PathManager.cpp
 * @brief Implements the PathManager utility class.
 */
#include "PathManager.hpp"
#include "../Constants.hpp"
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <ShlObj.h> // For SHGetFolderPathW
#else
#include <pwd.h>    // For getpwuid
#include <unistd.h> // For getuid
#endif

namespace { // Anonymous namespace for internal helpers

fs::path GetExecutablePath() {
#ifdef _WIN32
    WCHAR path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return fs::path(path);
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
#endif
}

// Helper function to get the user's documents directory in a cross-platform way.
fs::path GetUserDocumentsDirectory() {
#ifdef _WIN32
    WCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, 0, path))) {
        return fs::path(path);
    }
#else
    const char* home_dir = getenv("HOME");
    if (home_dir == nullptr) {
        // Fallback for environments where HOME is not set
        struct passwd* pw = getpwuid(getuid());
        if (pw != nullptr) {
            home_dir = pw->pw_dir;
        }
    }
    if (home_dir != nullptr) {
        fs::path docs_path = fs::path(home_dir) / "Documents";
        // Check if a "Documents" directory exists, otherwise just use home.
        if (fs::exists(docs_path) && fs::is_directory(docs_path)) {
            return docs_path;
        }
        return fs::path(home_dir);
    }
#endif
    // Final fallback to the current path if all else fails.
    return fs::current_path();
}
} // end anonymous namespace

PathManager::PathManager(const ProgramOptions& opts) {
    
    //Determine application directory once upon construction.
    m_app_directory = GetExecutablePath().parent_path();
    
    fs::path full_csv_path(opts.output_filename);
    // If the provided path has no parent (it's just a filename like "results.csv"),
    // then we build the path inside the user's Documents directory.
    if (full_csv_path.parent_path().empty()) {
        m_output_directory = GetUserDocumentsDirectory();
        m_csv_filename = full_csv_path.filename();
    } else {
        // Otherwise, the user has provided a relative or absolute path, so we respect it.
        m_output_directory = full_csv_path.parent_path();
        m_csv_filename = full_csv_path.filename();
    }
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

    // Add the correct extension based on the global constant.
    std::string extension;
    switch (DynaRange::Constants::PLOT_FORMAT) {
        case DynaRange::Constants::PlotOutputFormat::PDF: extension = ".pdf"; break;
        case DynaRange::Constants::PlotOutputFormat::SVG: extension = ".svg"; break;
        case DynaRange::Constants::PlotOutputFormat::PNG:
        default: extension = ".png"; break;
    }
    new_filename_ss << extension;
    return m_output_directory / new_filename_ss.str();
}

fs::path PathManager::GetSummaryPlotPath(const std::string& camera_name) const {
    std::string safe_camera_name = camera_name;
    std::replace(safe_camera_name.begin(), safe_camera_name.end(), ' ', '_');
    
    // Add the correct extension based on the global constant.
    std::string extension;
    switch (DynaRange::Constants::PLOT_FORMAT) {
        case DynaRange::Constants::PlotOutputFormat::PDF: extension = ".pdf"; break;
        case DynaRange::Constants::PlotOutputFormat::SVG: extension = ".svg"; break;
        case DynaRange::Constants::PlotOutputFormat::PNG:
        default: extension = ".png"; break;
    }
    std::string filename = "DR_summary_plot_" + safe_camera_name + extension;
    return m_output_directory / filename;
}

fs::path PathManager::GetAppDirectory() const {
    return m_app_directory;
}

fs::path PathManager::GetLocaleDirectory() const {
    return m_app_directory / "locale";
}

fs::path PathManager::GetAssetPath(const std::string& asset_name) const {
    return m_app_directory / asset_name;
}