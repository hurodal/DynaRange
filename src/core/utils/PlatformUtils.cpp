// File: src/core/utils/PlatformUtils.cpp
/**
 * @file src/core/utils/PlatformUtils.cpp
 * @brief Implements platform-specific utility functions.
 */
#include "PlatformUtils.hpp"

#ifdef _WIN32
#include <windows.h>
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace PlatformUtils {

// The implementation is compiled conditionally. On non-Windows platforms,
// this function will simply return the original list of files.
std::vector<std::string> ExpandWildcards(const std::vector<std::string>& files) {
#ifdef _WIN32
    // --- Windows-specific implementation ---
    auto expand_single_wildcard = [](const std::string& pattern, std::vector<std::string>& expanded_files) {
        WIN32_FIND_DATAA find_data;
        HANDLE h_find = FindFirstFileA(pattern.c_str(), &find_data);
        if (h_find != INVALID_HANDLE_VALUE) {
            fs::path pattern_path(pattern);
            fs::path parent_dir = pattern_path.parent_path();
            do {
                if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    expanded_files.push_back((parent_dir / find_data.cFileName).string());
                }
            } while (FindNextFileA(h_find, &find_data) != 0);
            FindClose(h_find);
        }
    };

    std::vector<std::string> result_files;
    for (const auto& file_arg : files) {
        if (file_arg.find_first_of("*?") != std::string::npos) {
            expand_single_wildcard(file_arg, result_files);
        } else {
            result_files.push_back(file_arg);
        }
    }
    return result_files;
#else
    // --- Non-Windows implementation ---
    // On Linux/macOS, the shell expands wildcards automatically, so we just return the original vector.
    return files;
#endif
}

} // namespace PlatformUtils