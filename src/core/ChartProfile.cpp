/**
 * @file core/ChartProfile.cpp
 * @brief Implements the ChartProfile class.
 */
#include "ChartProfile.hpp"

ChartProfile::ChartProfile() 
    : m_grid_cols(11), m_grid_rows(7) 
{
    // Hardcoded points for the specific magenta chart used.
    m_corner_points = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};

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