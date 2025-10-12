// File: src/core/graphics/detection/ChartCornerDetector.cpp
/**
 * @file ChartCornerDetector.cpp
 * @brief Implements the chart corner detection algorithm.
 */
#include "ChartCornerDetector.hpp"
#include "../../math/Math.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <cmath>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace DynaRange::Graphics::Detection {

std::optional<std::vector<cv::Point2d>> DetectChartCorners(const cv::Mat& bayer_image, std::ostream& log_stream)
{
    if (bayer_image.empty()) return std::nullopt;
    const int DIMX = bayer_image.cols;
    const int DIMY = bayer_image.rows;
    std::vector<cv::Point2d> detected_points;
    std::vector<cv::Rect> sectors = {
        {0, 0, DIMX / 2, DIMY / 2},
        {0, DIMY / 2, DIMX / 2, DIMY / 2},
        {DIMX / 2, DIMY / 2, DIMX / 2, DIMY / 2},
        {DIMX / 2, 0, DIMX / 2, DIMY / 2}
    };
    const double diag = std::sqrt(static_cast<double>(DIMX * DIMX + DIMY * DIMY));
    const double radius = diag * 0.01;
    const double circle_area = M_PI * radius * radius;
    const double quadrant_area = (DIMX / 2.0) * (DIMY / 2.0);
    const double quantile_fraction = circle_area / quadrant_area;
    const double quantile_threshold = 1.0 - (quantile_fraction / 4.0);
    for (const auto& sector_rect : sectors) {
        cv::Mat quadrant = bayer_image(sector_rect).clone();
        if (quadrant.empty() || quadrant.total() == 0) continue;

        std::vector<double> pixels;
        quadrant.reshape(1, 1).convertTo(pixels, CV_64F);
        double brightness_q_threshold = CalculateQuantile(pixels, quantile_threshold);

        cv::Mat mask;
        cv::threshold(quadrant, mask, brightness_q_threshold, 1.0, cv::THRESH_BINARY);
        mask.convertTo(mask, CV_8U);
        
        std::vector<cv::Point> bright_pixels;
        cv::findNonZero(mask, bright_pixels);
        if (bright_pixels.empty()) {
            log_stream << _("Warning: No corner circle found in one of the quadrants.") << std::endl;
            return std::nullopt;
        }

        std::vector<int> x_coords, y_coords;
        x_coords.reserve(bright_pixels.size());
        y_coords.reserve(bright_pixels.size());
        for(const auto& p : bright_pixels) {
            x_coords.push_back(p.x);
            y_coords.push_back(p.y);
        }
        std::sort(x_coords.begin(), x_coords.end());
        std::sort(y_coords.begin(), y_coords.end());
        
        double median_x_local = static_cast<double>(x_coords[x_coords.size() / 2]);
        double median_y_local = static_cast<double>(y_coords[y_coords.size() / 2]);
        
        double median_x = median_x_local + sector_rect.x;
        double median_y = median_y_local + sector_rect.y;
        detected_points.emplace_back(median_x, median_y);
    }
    
    if (detected_points.size() == 4) {
        cv::Point2d tl = detected_points[0];
        cv::Point2d bl = detected_points[1];
        cv::Point2d br = detected_points[2];
        cv::Point2d tr = detected_points[3];
        return std::vector<cv::Point2d>{tl, bl, br, tr};
    }

    return std::nullopt;
}

} // namespace DynaRange::Graphics::Detection