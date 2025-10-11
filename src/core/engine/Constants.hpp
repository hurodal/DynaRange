// File: src/core/engine/Constants.hpp
/**
 * @file src/core/engine/Constants.hpp
 * @brief Centralizes constants related to the main processing engine workflow.
 */
#pragma once

namespace DynaRange::Engine::Constants {

    /**
     * @brief The minimum percentage of the total image area that the detected
     * chart must occupy to be considered valid.
     */
    constexpr double MINIMUM_CHART_AREA_PERCENTAGE = 0.30; // 30%

} // namespace DynaRange::Engine::Constants