// File: src/core/setup/ChartProfile.cpp
/**
 * @file core/ChartProfile.cpp
 * @brief Implements the ChartProfile class.
 */
#include "ChartProfile.hpp"
#include <algorithm> // For std::min_element and std::max_element
#include <iomanip>
#include <sstream>
#include <libintl.h>

#define _(string) gettext(string)

ChartProfile::ChartProfile(
    const std::vector<double>& chart_coords,
    int patches_m,
    int patches_n,
    const std::optional<std::vector<cv::Point2d>>& detected_corners,
    std::ostream& log_stream)
    : m_grid_cols(patches_n),
      m_grid_rows(patches_m)
{
    // Priority 1: Use manually provided coordinates.
    if (!chart_coords.empty() && chart_coords.size() == 8) {
        m_has_manual_coords = true;
        std::vector<cv::Point2d> points;
        for (size_t i = 0; i < 8; i += 2) {
            points.emplace_back(chart_coords[i] / 2.0, chart_coords[i + 1] / 2.0);
        }
        
        auto tl_it = std::min_element(points.begin(), points.end(),
            [](const cv::Point2d& a, const cv::Point2d& b) { return a.x + a.y < b.x + b.y; });
        auto br_it = std::max_element(points.begin(), points.end(),
            [](const cv::Point2d& a, const cv::Point2d& b) { return a.x + a.y < b.x + b.y; });
        
        constexpr double epsilon = 1e-6;
        auto bl_it = std::min_element(points.begin(), points.end(),
            [epsilon](const cv::Point2d& a, const cv::Point2d& b) {
                return a.x / (a.y + epsilon) < b.x / (b.y + epsilon);
            });
        auto tr_it = std::max_element(points.begin(), points.end(),
            [epsilon](const cv::Point2d& a, const cv::Point2d& b) {
                return a.x / (a.y + epsilon) < b.x / (b.y + epsilon);
            });

        m_corner_points = {*tl_it, *bl_it, *br_it, *tr_it};
        LogCornerPoints(m_corner_points, _("Using manually specified coordinates:"), log_stream);
    } 
    // Priority 2: Use auto-detected coordinates.
    else if (detected_corners.has_value()) {
        m_has_manual_coords = false;
        m_corner_points = *detected_corners;
        LogCornerPoints(m_corner_points, _("Using automatically detected coordinates:"), log_stream);
    } 
    // Priority 3: Fallback to hardcoded defaults.
    else {
        m_has_manual_coords = true; // Treat hardcoded as manual (no GAP).
        m_corner_points = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
        log_stream << _("Warning: Automatic corner detection failed or was not possible. Falling back to default coordinates.") << std::endl;
        LogCornerPoints(m_corner_points, _("Using hardcoded default coordinates:"), log_stream);
    }

    // This logic is correct and matches the R script's intent.
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

void ChartProfile::LogCornerPoints(const std::vector<cv::Point2d>& points, const std::string& source_msg, std::ostream& log_stream) const {
    log_stream << source_msg << std::endl;

    // Standard order is TL, BL, BR, TR
    const auto& tl = points[0];
    const auto& bl = points[1];
    const auto& br = points[2];
    const auto& tr = points[3];

    // Use stringstreams to build perfectly formatted lines
    std::stringstream header, line1, line2;
    
    // Build Header line with final corrected padding.
    header << std::string(7, ' ') // Mimics the width of "  TL-> "
           << " " // Mimics the width of "["
           << std::right << std::setw(5) << "x"
           << "  " // Mimics the width of ", "
           << std::right << std::setw(5) << "y"
           << "      " // Corrected: 6 spaces to mimic " ]   ["
           << std::right << std::setw(5) << "x"
           << "  " // Mimics the width of ", "
           << std::right << std::setw(5) << "y";

    // Build Top line (TL, TR) WITH brackets
    line1 << "  TL-> [" << std::right << std::setw(5) << static_cast<int>(round(tl.x * 2.0))
          << ", " << std::right << std::setw(5) << static_cast<int>(round(tl.y * 2.0)) << " ]   ["
          << std::right << std::setw(5) << static_cast<int>(round(tr.x * 2.0))
          << ", " << std::right << std::setw(5) << static_cast<int>(round(tr.y * 2.0)) << " ] <-TR";

    // Build Bottom line (BL, BR) WITH brackets
    line2 << "  BL-> [" << std::right << std::setw(5) << static_cast<int>(round(bl.x * 2.0))
          << ", " << std::right << std::setw(5) << static_cast<int>(round(bl.y * 2.0)) << " ]   ["
          << std::right << std::setw(5) << static_cast<int>(round(br.x * 2.0))
          << ", " << std::right << std::setw(5) << static_cast<int>(round(br.y * 2.0)) << " ] <-BR";
    
    // Print the formatted lines to the log stream
    log_stream << header.str() << std::endl;
    log_stream << line1.str() << std::endl;
    log_stream << line2.str() << std::endl;
}