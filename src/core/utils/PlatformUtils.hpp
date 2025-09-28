// File: src/core/utils/PlatformUtils.hpp
/**
 * @file src/core/utils/PlatformUtils.hpp
 * @brief Declares utility functions for platform-specific operations.
 * @details This module adheres to SRP by encapsulating logic that is specific
 * to a particular operating system, such as Windows-specific file handling.
 */
#pragma once

#include <string>
#include <vector>

namespace PlatformUtils {

/**
 * @brief Expands file patterns (wildcards like *) on Windows.
 * @details On non-Windows platforms, this function does nothing and returns the
 * input list as is.
 * @param files A vector of file paths, some of which may contain wildcards.
 * @return A new vector of strings with all wildcards expanded to actual file paths.
 */
std::vector<std::string> ExpandWildcards(const std::vector<std::string>& files);

} // namespace PlatformUtils