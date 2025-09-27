// File: src/core/ChartProfile.cpp
/**
 * @file core/ChartProfile.cpp
 * @brief Implements the ChartProfile class.
 */
#include "ChartProfile.hpp"
#include <algorithm> // For std::min_element and std::max_element

ChartProfile::ChartProfile(const ProgramOptions& opts)

    : m_grid_cols(opts.chart_patches.size() >= 2 ? opts.chart_patches[1] : 11),
      m_grid_rows(opts.chart_patches.size() >= 1 ? opts.chart_patches[0] : 7)
{
    if (!opts.chart_coords.empty() && opts.chart_coords.size() == 8) {
        // - Use and Reorder User-Provided Coordinates -
        m_has_manual_coords = true;
        std::vector<cv::Point2d> points;
        for (size_t i = 0; i < 8; i += 2) {
            points.emplace_back(opts.chart_coords[i] / 2.0, opts.chart_coords[i + 1] / 2.0);
        }

        // Reorder points to ensure correct TL, BL, BR, TR order, regardless of user input.
        auto tl_it = std::min_element(points.begin(), points.end(), 
            [](const cv::Point2d& a, const cv::Point2d& b) { return a.x + a.y < b.x + b.y; });
        auto br_it = std::max_element(points.begin(), points.end(), 
            [](const cv::Point2d& a, const cv::Point2d& b) { return a.x + a.y < b.x + b.y; });
        
        constexpr double epsilon = 1e-6; // To prevent division by zero
        
        auto bl_it = std::min_element(points.begin(), points.end(),
            [epsilon](const cv::Point2d& a, const cv::Point2d& b) {
                return a.x / (a.y + epsilon) < b.x / (b.y + epsilon);
            });
            
        auto tr_it = std::max_element(points.begin(), points.end(),
            [epsilon](const cv::Point2d& a, const cv::Point2d& b) {
                return a.x / (a.y + epsilon) < b.x / (b.y + epsilon);
            });
            
        m_corner_points = {*tl_it, *bl_it, *br_it, *tr_it};

    } else {
        // --- Fallback to Hardcoded Default Coordinates ---
        m_has_manual_coords = false;
        m_corner_points = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
    }

    // --- Calculate Destination Points for Keystone Correction ---
    double xtl = (m_corner_points[0].x + m_corner_points[1].x) / 2.0;
    double ytl = (m_corner_points[0].y + m_corner_points[3].y) / 2.0;
    double xbr = (m_corner_points[2].x + m_corner_points[3].x) / 2.0;
    double ybr = (m_corner_points[1].y + m_corner_points[2].y) / 2.0;
    
    m_destination_points = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
}

const std::vector<cv::Point2d>& ChartProfile::GetCornerPoints() const {
    return m_corner_points;
}

const std::vector<cv::Point2d>& ChartProfile::GetDestinationPoints() const {
    return m_destination_points;
}

int ChartProfile::GetGridCols() const {
    return m_grid_cols;
}

int ChartProfile::GetGridRows() const {
    return m_grid_rows;
}

bool ChartProfile::HasManualCoords() const {
    return m_has_manual_coords;
}