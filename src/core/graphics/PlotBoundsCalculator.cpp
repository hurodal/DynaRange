// File: src/core/graphics/PlotBoundsCalculator.cpp
/**
 * @file PlotBoundsCalculator.cpp
 * @brief Implements the plot axis boundary calculation utility.
 */
#include "PlotBoundsCalculator.hpp"
#include <algorithm>
#include <cmath>

namespace DynaRange::Graphics {

std::map<std::string, double> CalculateGlobalBounds(const std::vector<CurveData>& curves) {
    double min_ev_global = 1e6, max_ev_global = -1e6;
    double min_db_global = 1e6, max_db_global = -1e6;

    for (const auto& curve : curves) {
        if (!curve.points.empty()) {
            auto minmax_ev_it = std::minmax_element(curve.points.begin(), curve.points.end(), [](const PointData& a, const PointData& b) { return a.ev < b.ev; });
            min_ev_global = std::min(min_ev_global, minmax_ev_it.first->ev);
            max_ev_global = std::max(max_ev_global, minmax_ev_it.second->ev);

            auto minmax_db_it = std::minmax_element(curve.points.begin(), curve.points.end(), [](const PointData& a, const PointData& b) { return a.snr_db < b.snr_db; });
            min_db_global = std::min(min_db_global, minmax_db_it.first->snr_db);
            max_db_global = std::max(max_db_global, minmax_db_it.second->snr_db);
        }
    }

    std::map<std::string, double> bounds;
    if (min_ev_global > max_ev_global) { // Case where there's no data
        bounds["min_ev"] = -15.0;
        bounds["max_ev"] = 1.0;
        bounds["min_db"] = -20.0;
        bounds["max_db"] = 40.0;
    } else {
        bounds["min_ev"] = floor(min_ev_global) - 1.0;
        bounds["max_ev"] = (max_ev_global < 0.0) ? 1.0 : ceil(max_ev_global) + 1.0;
        bounds["min_db"] = floor(min_db_global / 5.0) * 5.0;
        bounds["max_db"] = ceil(max_db_global / 5.0) * 5.0;
    }
    return bounds;
}

} // namespace DynaRange::Graphics