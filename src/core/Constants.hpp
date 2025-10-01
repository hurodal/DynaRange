// File: src/core/Constants.hpp
/**
 * @file src/core/Constants.hpp
 * @brief Centralizes global constants and definitions not tied to argument defaults.
 */
#pragma once

namespace DynaRange::Constants {

    // Nombre del ejecutable para la línea de comandos.
    constexpr const char* CLI_EXECUTABLE_NAME = "rango";

    /**
     * @brief The minimum percentage of the total image area that the detected
     * chart must occupy to be considered valid.
     */
    constexpr double MINIMUM_CHART_AREA_PERCENTAGE = 0.50; // 50%

} // namespace DynaRange::Constants