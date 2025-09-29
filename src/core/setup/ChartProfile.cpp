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

ChartProfile::ChartProfile(const ProgramOptions& opts, const std::optional<std::vector<cv::Point2d>>& detected_corners, std::ostream& log_stream)
    : m_grid_cols(opts.GetChartPatchesN()),
      m_grid_rows(opts.GetChartPatchesM())
{
    // Priority 1: Use manually provided coordinates if they exist.
    if (!opts.chart_coords.empty() && opts.chart_coords.size() == 8) {
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

        // CORRECTED LOGIC: Use division (x/y) to find BL and TR corners, matching the R script.
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
    
    // Priority 2: Use auto-detected coordinates if detection was successful.
    } else if (detected_corners.has_value()) {
        m_has_manual_coords = false;
        m_corner_points = *detected_corners;
        LogCornerPoints(m_corner_points, _("Using automatically detected coordinates:"), log_stream);

    // Priority 3: Fallback to hardcoded defaults if all else fails.
    } else {
        m_has_manual_coords = false;
        m_corner_points = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
        log_stream << _("Warning: Automatic corner detection failed or was not possible. Falling back to default coordinates.") << std::endl;
        LogCornerPoints(m_corner_points, _("Using hardcoded default coordinates:"), log_stream);
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

void ChartProfile::LogCornerPoints(const std::vector<cv::Point2d>& points, const std::string& source_msg, std::ostream& log_stream) const {
    log_stream << source_msg << std::endl;

    auto format_point_str = [](const cv::Point2d& p) {
        std::stringstream ss;
        ss << "[" << std::right << std::setw(5) << static_cast<int>(round(p.x * 2.0))
           << ", " << std::right << std::setw(5) << static_cast<int>(round(p.y * 2.0)) << " ]";
        return ss.str();
    };

    // Standard order is TL, BL, BR, TR
    std::string tl_str = "  TL-> " + format_point_str(points[0]);
    std::string tr_str = format_point_str(points[3]) + " <-TR";
    std::string bl_str = "  BL-> " + format_point_str(points[1]);
    std::string br_str = format_point_str(points[2]) + " <-BR";

    log_stream << tl_str << "\t" << tr_str << std::endl;
    log_stream << bl_str << "\t" << br_str << std::endl;
}