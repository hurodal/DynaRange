// File: src/gui/helpers/RawExtensionHelper.hpp
/**
 * @file RawExtensionHelper.hpp
 * @brief Declares a helper utility for retrieving supported RAW file extensions.
 */
#pragma once
#include <vector>
#include <string>

namespace GuiHelpers {

/**
 * @brief Gets a list of supported RAW file extensions.
 * @details It first attempts to query LibRaw dynamically. If that fails or is
 * not supported, it falls back to a hardcoded list. The result is cached.
 * @return A constant reference to a vector of extension strings (e.g., "arw", "cr3").
 */
const std::vector<std::string>& GetSupportedRawExtensions();

} // namespace GuiHelpers