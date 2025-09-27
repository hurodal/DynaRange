// File: core/ChartProfile.hpp
/**
 * @file core/ChartProfile.hpp
 * @brief Defines a profile for a test chart's geometric properties.
 */
#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include "arguments/ProgramOptions.hpp" // Needed for the constructor

/**
 * @class ChartProfile
 * @brief Encapsulates the geometric properties of a specific test chart.
 * @details This class removes hardcoded values from the processing engine by
 * providing a single source for chart-specific data like corner points,
 * crop areas, and patch grid dimensions. It can be initialized with default
 * values or with user-provided coordinates.
 */
class ChartProfile {
public:
    /**
     * @brief Constructs a chart profile based on program options.
     * @details If manual coordinates are provided in opts, they are used.
     * Otherwise, hardcoded default coordinates are used as a fallback.
     * @param opts The program options containing potential manual coordinates.
     */
    explicit ChartProfile(const ProgramOptions& opts);

    /// @brief Gets the four corner points of the chart for keystone correction.
    const std::vector<cv::Point2d>& GetCornerPoints() const;

    /// @brief Gets the target destination points for keystone correction.
    const std::vector<cv::Point2d>& GetDestinationPoints() const;

    /// @brief Gets the number of patch columns in the chart grid.
    int GetGridCols() const;

    /// @brief Gets the number of patch rows in the chart grid.
    int GetGridRows() const;
    
    /// @brief Checks if the profile was constructed with user-provided coordinates.
    bool HasManualCoords() const;

private:
    std::vector<cv::Point2d> m_corner_points;      ///< Source points for keystone.
    std::vector<cv::Point2d> m_destination_points; ///< Destination points for keystone.
    int m_grid_cols;                               ///< Number of columns of patches.
    int m_grid_rows;                               ///< Number of rows of patches.
    bool m_has_manual_coords = false;              ///< True if user-provided coords were used.
};