// File: src/core/utils/PathManager.cpp
/**
 * @file core/utils/PathManager.cpp
 * @brief Implements the PathManager utility class.
 */
#include "PathManager.hpp"

#ifdef _WIN32
#include <ShlObj.h> // For SHGetFolderPathW
#include <windows.h> // For GetModuleFileNameW, MAX_PATH
#else
#include <pwd.h>    // For getpwuid
#include <unistd.h> // For getuid, readlink
#include <limits.h> // For PATH_MAX
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> // For _NSGetExecutablePath
#include <limits.h>
#endif

namespace { // Anonymous namespace for internal helpers

    /**
     * @brief Gets the full path to the currently running executable.
     * @return Filesystem path to the executable, or empty path on error.
     */
    fs::path GetExecutablePath() {
        #ifdef _WIN32
            WCHAR path[MAX_PATH] = {0};
            if (GetModuleFileNameW(NULL, path, MAX_PATH) > 0) {
                 return fs::path(path);
            }
        #elif __APPLE__
            char path[PATH_MAX];
            uint32_t size = sizeof(path);
            if (_NSGetExecutablePath(path, &size) == 0) {
                char real_path[PATH_MAX];
                if (realpath(path, real_path)) {
                    return fs::path(real_path);
                }
                // Fallback if realpath fails
                return fs::path(path);
            }
        #else // Linux
            char result[PATH_MAX];
            ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
            if (count > 0) {
                 return std::string(result, count);
            }
        #endif
        return ""; // Return empty on error
    }

    /**
     * @brief Gets the user's standard "Documents" directory in a cross-platform way.
     * @return Filesystem path to the Documents directory, falling back to home or current path.
     */
    fs::path GetUserDocumentsDirectory() {
        #ifdef _WIN32
            WCHAR path[MAX_PATH];
            // Use FOLDERID_Documents for modern Windows versions
            // Fallback to CSIDL_MYDOCUMENTS if needed, though FOLDERID is preferred
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, path))) {
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
                fs::path home_path(home_dir);
                // Standard XDG directory or common fallbacks
                fs::path xdg_docs_path = home_path / "Documents"; // Standard XDG
                fs::path mac_docs_path = home_path / "Documents"; // Common macOS/Linux

                // Check standard locations first
                if (fs::exists(xdg_docs_path) && fs::is_directory(xdg_docs_path)) {
                    return xdg_docs_path;
                }
                 if (fs::exists(mac_docs_path) && fs::is_directory(mac_docs_path)) {
                    return mac_docs_path;
                }
                // If specific "Documents" folder doesn't exist, return the home directory itself
                return home_path;
            }
        #endif
        // Final fallback to the current working directory if all else fails.
        return fs::current_path();
    }
} // end anonymous namespace

PathManager::PathManager(const ProgramOptions& opts) : m_opts(opts) {

    // Determine application directory once upon construction.
    m_app_directory = GetExecutablePath().parent_path();

    // Determine the base output directory based on opts.output_filename
    fs::path user_output_path(opts.output_filename);

    // If the path provided by the user is absolute, use its parent directory.
    if (user_output_path.is_absolute()) {
        m_output_directory = user_output_path.parent_path();
    }
    // If the path has a parent (e.g., "subdir/results.csv"), resolve it relative to CWD.
    else if (user_output_path.has_parent_path() && !user_output_path.parent_path().empty()) {
         m_output_directory = fs::absolute(user_output_path.parent_path());
    }
    // If it's just a filename (no parent), use the user's Documents directory.
    else {
        m_output_directory = GetUserDocumentsDirectory();
    }

    // Ensure the output directory exists.
    try {
        if (!fs::exists(m_output_directory)) {
            fs::create_directories(m_output_directory);
        }
    } catch (const fs::filesystem_error& e) {
        // Log error? For now, fallback to current path if creation fails.
        m_output_directory = fs::current_path();
    }
}

fs::path PathManager::GetOutputDirectory() const {
    return m_output_directory;
}

fs::path PathManager::GetFullPath(const fs::path& generated_filename) const {
    // Ensure the filename part is treated as relative if it's not absolute already
    if (generated_filename.is_absolute()) {
        return generated_filename; // User provided an absolute path override
    }
    return m_output_directory / generated_filename;
}

fs::path PathManager::GetAppDirectory() const {
    return m_app_directory;
}

fs::path PathManager::GetLocaleDirectory() const {
    // Consider build type (install vs build dir) if needed later
    return m_app_directory / "locale";
}

fs::path PathManager::GetAssetPath(const std::string& asset_name) const {
    // Simply join the application directory with the provided relative asset name/path.
    // The caller is responsible for providing the correct relative path including "assets/".
    return m_app_directory / asset_name;
}