/**
 * @file core/ChartProfile.hpp
 * @brief Defines a profile for a test chart's geometric properties.
 */
#pragma once

#include <vector>
#include <opencv2/core.hpp>

/**
 * @class ChartProfile
 * @brief Encapsulates the geometric properties of a specific test chart.
 * @details This class removes hardcoded values from the processing engine by
 * providing a single source for chart-specific data like corner points,
 * crop areas, and patch grid dimensions.
 */
class ChartProfile {
public:
    ChartProfile();

    /// @brief Gets the four corner points of the chart for keystone correction.
    const std::vector<cv::Point2d>& GetCornerPoints() const;

    /// @brief Gets the target destination points for keystone correction.
    const std::vector<cv::Point2d>& GetDestinationPoints() const;
    
    /// @brief Gets the number of patch columns in the chart grid.
    int GetGridCols() const;

    /// @brief Gets the number of patch rows in the chart grid.
    int GetGridRows() const;

private:
    std::vector<cv::Point2d> m_corner_points;      ///< Source points for keystone.
    std::vector<cv::Point2d> m_destination_points; ///< Destination points for keystone.
    int m_grid_cols;                               ///< Number of columns of patches.
    int m_grid_rows;                               ///< Number of rows of patches.
};