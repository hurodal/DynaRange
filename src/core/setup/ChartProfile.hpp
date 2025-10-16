// File: src/core/setup/ChartProfile.hpp
/**
 * @file core/setup/ChartProfile.hpp
 * @brief Defines a profile for a test chart's geometric properties.
 */
#pragma once

#include <vector>
#include <optional>
#include <opencv2/core.hpp>
#include <ostream>

/**
 * @class ChartProfile
 * @brief Encapsulates the geometric properties of a specific test chart.
 */
class ChartProfile {
public:
    /**
     * @brief Constructs a chart profile based on program options and auto-detection results.
     * @param chart_coords Manually specified coordinates from user arguments.
     * @param patches_m The number of rows in the patch grid.
     * @param patches_n The number of columns in the patch grid.
     * @param detected_corners An optional vector with coordinates from automatic detection.
     * @param log_stream The output stream for logging messages.
     */
    explicit ChartProfile(
        const std::vector<double>& chart_coords,
        int patches_m,
        int patches_n,
        const std::optional<std::vector<cv::Point2d>>& detected_corners,
        std::ostream& log_stream
    );

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
    /**
     * @brief Logs the corner coordinates used for the analysis.
     * @param points The vector of 4 corner points (TL, BL, BR, TR).
     * @param source_msg A message indicating the source of the coordinates.
     * @param log_stream The output stream for logging.
     */
    void LogCornerPoints(const std::vector<cv::Point2d>& points, const std::string& source_msg, std::ostream& log_stream) const;

    std::vector<cv::Point2d> m_corner_points;      ///< Source points for keystone.
    std::vector<cv::Point2d> m_destination_points; ///< Destination points for keystone.
    int m_grid_cols;                               ///< Number of columns of patches.
    int m_grid_rows;                               ///< Number of rows of patches.
    bool m_has_manual_coords = false;              ///< True if user-provided coords were used.
};