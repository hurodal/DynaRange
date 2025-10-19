// File: src/core/utils/PlotTitleGenerator.hpp
/**
 * @file src/core/utils/PlotTitleGenerator.hpp
 * @brief Declares a class to generate standardized plot titles.
 */
#pragma once

#include "OutputNamingContext.hpp"
#include <string>

class PlotTitleGenerator {
public:
    /**
     * @brief Generates the title for the summary plot.
     * @param ctx The context containing necessary information (e.g., camera name).
     * @return The formatted title string (e.g., "SNR Curves (OM-1)").
     */
    static std::string GenerateSummaryTitle(const OutputNamingContext& ctx);

    /**
     * @brief Generates the title for an individual ISO plot.
     * @param ctx The context containing necessary information (e.g., camera name, ISO).
     * @return The formatted title string (e.g., "SNR Curve (OM-1, ISO 200)").
     * Returns an empty string if ISO is not available in the context.
     */
    static std::string GenerateIndividualTitle(const OutputNamingContext& ctx);
};