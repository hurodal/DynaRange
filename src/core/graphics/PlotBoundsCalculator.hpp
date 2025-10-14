// File: src/core/graphics/PlotBoundsCalculator.hpp
/**
 * @file PlotBoundsCalculator.hpp
 * @brief Declares a utility for calculating the axis boundaries for plots.
 * @details This module's single responsibility is to determine the optimal
 * min/max EV and SNR values to ensure all data points are visible across
 * one or more plots.
 */
#pragma once

#include "../analysis/Analysis.hpp"
#include <map>
#include <string>
#include <vector>

namespace DynaRange::Graphics {

/**
 * @brief Calculates the global axis boundaries across a collection of curves.
 * @param curves A vector of CurveData structs, which must have their curve_points generated.
 * @return A map containing the final plot boundaries (min_ev, max_ev, min_db, max_db).
 */
std::map<std::string, double> CalculateGlobalBounds(const std::vector<CurveData>& curves);

} // namespace DynaRange::Graphics