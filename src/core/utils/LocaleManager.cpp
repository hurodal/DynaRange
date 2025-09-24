// File: src/utils/LocaleManager.cpp
// =============================================================================
/**
 * @file LocaleManager.cpp
 * @brief Implements the LocaleManager class.
 */

#include "LocaleManager.hpp"
#include <cstring> // For strlen

LocaleManager::LocaleManager() {
    // Save the current numeric locale.
    char* current_locale = std::setlocale(LC_NUMERIC, nullptr);
    if (current_locale != nullptr && std::strlen(current_locale) > 0) {
        m_original_locale = current_locale;
    } else {
        // Fallback if the current locale is not set or is empty.
        m_original_locale = "C";
    }

    // Set the numeric locale to "C" for consistent parsing.
    std::setlocale(LC_NUMERIC, "C");
}

LocaleManager::~LocaleManager() {
    // Restore the original numeric locale.
    std::setlocale(LC_NUMERIC, m_original_locale.c_str());
}
// =============================================================================