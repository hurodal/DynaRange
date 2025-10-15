// File: src/core/setup/Constants.hpp
/**
 * @file src/core/setup/Constants.hpp
 * @brief Centralizes constants related to the pre-analysis and setup phase.
 */
#pragma once

namespace DynaRange::Setup::Constants {

    /**
     * @brief Maximum ratio of saturated pixels allowed for a RAW file to be
     * considered a valid source for corner detection.
     * @details A value of 0.001 means the file is flagged as "saturated" if 0.1%
     * or more of its pixels are at or above 99% of the saturation level.
     */
    constexpr double MAX_PRE_ANALYSIS_SATURATION_RATIO = 0.001; // 0.1%

} // namespace DynaRange::Setup::Constants