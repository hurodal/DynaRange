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

    /**
     * @enum BayerChannel
     * @brief Defines the channels in a standard RGGB Bayer pattern.
     */
    enum class BayerChannel { R = 0, G1 = 1, G2 = 2, B = 3 };

    /**
     * @brief The Bayer channel to be used for the dynamic range analysis.
     * @details 0 = R, 1 = G1 (first green), 2 = G2 (second green), 3 = B.
     */
    constexpr BayerChannel BAYER_CHANNEL_TO_ANALYZE = BayerChannel::B;

    // --- Patch Filtering Thresholds ---

    /**
     * @brief Minimum Signal-to-Noise Ratio (in dB) for a patch to be considered valid.
     * @details Aligned with the R script's reference value to include deep shadow data.
     */
    constexpr double MIN_SNR_DB_THRESHOLD = -90.0;

    /**
     * @brief Maximum ratio of saturated pixels for a patch to be considered valid.
     * @details A value of 0.01 means the patch is discarded if 1% or more of its pixels are saturated.
     */
    constexpr double MAX_SATURATION_RATIO = 0.01;


} // namespace DynaRange::Constants