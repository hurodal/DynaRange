// File: src/utils/LocaleManager.hpp
// =============================================================================
/**
 * @file LocaleManager.hpp
 * @brief Declares a RAII helper class for managing the program's numeric locale.
 * 
 * This class encapsulates the logic for setting the numeric locale to "C"
 * for consistent number parsing and restoring the original locale on destruction.
 * It adheres to the Single Responsibility Principle (SRP).
 */

#pragma once
#include <string>

class LocaleManager {
public:
    /**
     * @brief Constructor.
     * Saves the current LC_NUMERIC locale and sets it to "C".
     */
    LocaleManager();

    /**
     * @brief Destructor.
     * Restores the original LC_NUMERIC locale that was active at construction.
     */
    ~LocaleManager();

private:
    std::string m_original_locale;
};
// =============================================================================