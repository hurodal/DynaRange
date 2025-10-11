// File: src/core/analysis/Constants.hpp
/**
 * @file src/core/analysis/Constants.hpp
 * @brief Centralizes constants related to the image analysis process.
 */
#pragma once

namespace DynaRange::Analysis::Constants {

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

} // namespace DynaRange::Analysis::Constants