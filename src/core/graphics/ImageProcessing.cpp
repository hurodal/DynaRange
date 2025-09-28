// File: src/core/graphics/ImageProcessing.cpp
/**
 * @file core/ImageProcessing.cpp
 * @brief Implements geometric image processing functions.
 */
#include "ImageProcessing.hpp"
#include "../io/RawFile.hpp"
#include "../math/Math.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <optional>

#define _(string) gettext(string)

Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points) {
    Eigen::Matrix<double, 8, 8> A;
    Eigen::Vector<double, 8> b;
    for (int i = 0; i < 4; ++i) {
        const auto& xu = src_points[i].x;
        const auto& yu = src_points[i].y;
        const auto& xd = dst_points[i].x; const auto& yd = dst_points[i].y;
        A.row(2 * i)     << xd, yd, 1, 0,  0,  0, -xd * xu, -yd * xu;
        A.row(2 * i + 1) << 0,  0,  0, xd, yd, 1, -xd * yu, -yd * yu;
        b(2 * i) = xu; b(2 * i + 1) = yu;
    }
    return A.colPivHouseholderQr().solve(b);
}

/**
 * @brief Applies an inverse keystone correction to an image using Nearest Neighbor interpolation.
 * @param imgSrc The source image to be corrected.
 * @param k An Eigen::VectorXd containing the 8 transformation parameters.
 * @return A new cv::Mat containing the rectified image.
 */
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k) {
    int DIMX = imgSrc.cols;
    int DIMY = imgSrc.rows;
    cv::Mat imgCorrected = cv::Mat::zeros(DIMY, DIMX, CV_32FC1);

    for (int y = 0; y < DIMY; ++y) {
        for (int x = 0; x < DIMX; ++x) {
            double xd = x + 1.0, yd = y + 1.0;
            // RESTORED LOGIC: Use integer literal '1' for denominator.
            double denom = k(6) * xd + k(7) * yd + 1;
            if (std::abs(denom) < 1e-9) continue;

            // RESTORED LOGIC: Subtract 1.0 from the coordinate BEFORE rounding.
            double xu = (k(0) * xd + k(1) * yd + k(2)) / denom - 1;
            double yu = (k(3) * xd + k(4) * yd + k(5)) / denom - 1;
            
            // RESTORED LOGIC: Round the final coordinate without a second subtraction.
            int x_src = static_cast<int>(round(xu));
            int y_src = static_cast<int>(round(yu));

            if (x_src >= 0 && x_src < DIMX && y_src >= 0 && y_src < DIMY) {
                imgCorrected.at<float>(y, x) = imgSrc.at<float>(y_src, x_src);
            }
        }
    }
    return imgCorrected;
}

cv::Mat PrepareChartImage(
    const RawFile& raw_file, 
    const ProgramOptions& opts, 
    const Eigen::VectorXd& keystone_params,
    const ChartProfile& chart, 
    std::ostream& log_stream) 
{
    cv::Mat raw_img = raw_file.GetRawImage();
    if(raw_img.empty()){
        log_stream << _("Error: Could not get raw image for: ") << raw_file.GetFilename() << std::endl;
        return {};
    }
    cv::Mat img_float = NormalizeRawImage(raw_img, opts.dark_value, opts.saturation_value);
    
    // This logic now consistently extracts the G1 (Green) channel, which is
    // used for both corner detection and patch analysis, aligning with the R script.
    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    for (int r = 0; r < imgBayer.rows; ++r) {
        for (int c = 0; c < imgBayer.cols; ++c) {
            imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2 + 1);
        }
    }
    
    cv::Mat img_corrected = UndoKeystone(imgBayer, keystone_params);
    const auto& dst_pts = chart.GetDestinationPoints();

    double xtl = dst_pts[0].x;
    double ytl = dst_pts[0].y;
    double xbr = dst_pts[2].x;
    double ybr = dst_pts[2].y;
    cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));

    if (crop_area.x < 0 || crop_area.y < 0 || crop_area.width <= 0 || crop_area.height <= 0 ||
        crop_area.x + crop_area.width > img_corrected.cols ||
        crop_area.y + crop_area.height > img_corrected.rows) {
        log_stream << _("Error: Invalid crop area calculated for keystone correction.") << std::endl;
        return {};
    }

    return img_corrected(crop_area);
}

/*
// --- REFERENCE IMPLEMENTATION (Commented Out) ---
// This is a complete, separate version of the function that contains the logic
// aligned with the reference R script. It is kept here for documentation and
// future reference. It produces slightly different numerical results due to a
// more sophisticated cropping method that uses a safety "gap".
cv::Mat PrepareChartImage(
    const RawFile& raw_file, 
    const ProgramOptions& opts, 
    const Eigen::VectorXd& keystone_params,
    const ChartProfile& chart, 
    std::ostream& log_stream) 
{
    cv::Mat raw_img = raw_file.GetRawImage();
    if(raw_img.empty()){
        log_stream << _("Error: Could not get raw image for: ") << raw_file.GetFilename() << std::endl;
        return {};
    }
    cv::Mat img_float = NormalizeRawImage(raw_img, opts.dark_value, opts.saturation_value);
    
    // Extract a single bayer channel (e.g., Green) which is half the size.
    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    for (int r = 0; r < imgBayer.rows; ++r) {
        for (int c = 0; c < imgBayer.cols; ++c) {
            imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
        }
    }
    
    cv::Mat img_corrected = UndoKeystone(imgBayer, keystone_params);
    const auto& dst_pts = chart.GetDestinationPoints();
    
    // --- R-Script Aligned Cropping Logic ---
    // R script reference: `imgcrop=imgc[round(ytl+GAPY):round(ybr-GAPY), round(xtl+GAPX):round(xbr-GAPX)]`
    double gap_x = 0.0;
    double gap_y = 0.0;
    
    // If NOT using manual coords, apply a safety gap like the R script.
    if (!chart.HasManualCoords()) {
        const double xbr = dst_pts[2].x;
        const double xtl = dst_pts[0].x;
        const double ybr = dst_pts[2].y;
        const double ytl = dst_pts[0].y;
        // R script reference: `GAPX=(xbr-xtl) / (NCOLS+1) / 2`
        gap_x = (xbr - xtl) / (chart.GetGridCols() + 1) / 2.0;
        // R script reference: `GAPY=(ybr-ytl) / (NROWS+1) / 2`
        gap_y = (ybr - ytl) / (chart.GetGridRows() + 1) / 2.0;
    }
    
    cv::Rect crop_area(
        round(dst_pts[0].x + gap_x), 
        round(dst_pts[0].y + gap_y), 
        round(dst_pts[2].x - dst_pts[0].x - 2 * gap_x), 
        round(dst_pts[2].y - dst_pts[0].y - 2 * gap_y)
    );

    if (crop_area.x < 0 || crop_area.y < 0 || crop_area.width <= 0 || crop_area.height <= 0 ||
        crop_area.x + crop_area.width > img_corrected.cols ||
        crop_area.y + crop_area.height > img_corrected.rows) {
        log_stream << _("Error: Invalid crop area calculated for keystone correction.") << std::endl;
        return {};
    }

    return img_corrected(crop_area);
}
*/

cv::Mat NormalizeRawImage(const cv::Mat& raw_image, double black_level, double sat_level)
{
    if (raw_image.empty()) {
        return {};
    }
    cv::Mat float_img;
    raw_image.convertTo(float_img, CV_32F);

    // Normalize the image to a 0.0-1.0 range
    float_img = (float_img - black_level) / (sat_level - black_level);
    return float_img;
}

cv::Mat CreateFinalDebugImage(const cv::Mat& overlay_image, double max_pixel_value)
{
    if (overlay_image.empty() || max_pixel_value <= 0) {
        return {};
    }

    // 1. Normalize using the max value calculated *before* drawing patches.
    cv::Mat normalized_image = overlay_image / max_pixel_value;

    // 2. Clamp values to the [0.0, 1.0] range.
    cv::threshold(normalized_image, normalized_image, 1.0, 1.0, cv::THRESH_TRUNC);
    cv::threshold(normalized_image, normalized_image, 0.0, 0.0, cv::THRESH_TOZERO);

    // 3. Apply standard 2.2 gamma correction for visibility.
    cv::Mat gamma_corrected_image;
    cv::pow(normalized_image, 1.0 / 2.2, gamma_corrected_image);

    return gamma_corrected_image;
}

std::optional<std::vector<cv::Point2d>> DetectChartCorners(const cv::Mat& bayer_image, std::ostream& log_stream)
{
    if (bayer_image.empty()) return std::nullopt;

    const int DIMX = bayer_image.cols;
    const int DIMY = bayer_image.rows;
    std::vector<cv::Point2d> detected_points;

    // Define the four quadrants of the image
    std::vector<cv::Rect> sectors = {
        {0, 0, DIMX / 2, DIMY / 2},                             // Top-Left
        {0, DIMY / 2, DIMX / 2, DIMY / 2},                      // Bottom-Left
        {DIMX / 2, DIMY / 2, DIMX / 2, DIMY / 2},               // Bottom-Right
        {DIMX / 2, 0, DIMX / 2, DIMY / 2}                       // Top-Right
    };

    // R-Script logic: circle radius is 1% of the image diagonal.
    // This is used to calculate how many pixels to select (quantile).
    const double diag = std::sqrt(static_cast<double>(DIMX * DIMX + DIMY * DIMY));
    const double radius = diag * 0.01;
    const double circle_area = M_PI * radius * radius;
    const double quadrant_area = (DIMX / 2.0) * (DIMY / 2.0);
    const double quantile_threshold = 1.0 - (circle_area / quadrant_area);

    for (const auto& sector_rect : sectors) {
        // Added .clone() to create a continuous memory block for the quadrant.
        // This is necessary for the subsequent .reshape() call to work correctly.
        cv::Mat quadrant = bayer_image(sector_rect).clone();
        if (quadrant.empty() || quadrant.total() == 0) continue;

        // 1. Find the brightness threshold using a quantile
        std::vector<double> pixels;
        quadrant.reshape(1, 1).convertTo(pixels, CV_64F);
        double brightness_threshold = CalculateQuantile(pixels, quantile_threshold);

        // 2. Find all pixels brighter than the threshold
        cv::Mat mask;
        cv::threshold(quadrant, mask, brightness_threshold, 1.0, cv::THRESH_BINARY);
        mask.convertTo(mask, CV_8U);
        
        std::vector<cv::Point> bright_pixels;
        cv::findNonZero(mask, bright_pixels);

        if (bright_pixels.empty()) {
            log_stream << _("Warning: No corner circle found in one of the quadrants.") << std::endl;
            return std::nullopt;
        }

        // 3. Calculate the median coordinate
        std::vector<int> x_coords, y_coords;
        x_coords.reserve(bright_pixels.size());
        y_coords.reserve(bright_pixels.size());
        for(const auto& p : bright_pixels) {
            x_coords.push_back(p.x);
            y_coords.push_back(p.y);
        }
        std::sort(x_coords.begin(), x_coords.end());
        std::sort(y_coords.begin(), y_coords.end());

        // 4. Adjust local coordinates to full image space and store the point
        double median_x = static_cast<double>(x_coords[x_coords.size() / 2]) + sector_rect.x;
        double median_y = static_cast<double>(y_coords[y_coords.size() / 2]) + sector_rect.y;
        detected_points.emplace_back(median_x, median_y);
    }
    
    if (detected_points.size() == 4) {
        // Reorder points to TL, BL, BR, TR to match ChartProfile's expectations
        cv::Point2d tl = detected_points[0];
        cv::Point2d bl = detected_points[1];
        cv::Point2d br = detected_points[2];
        cv::Point2d tr = detected_points[3];
        return std::vector<cv::Point2d>{tl, bl, br, tr};
    }

    return std::nullopt;
}


cv::Mat UndoKeystoneColor(const cv::Mat& imgSrc, const Eigen::VectorXd& k) {
    int DIMX = imgSrc.cols;
    int DIMY = imgSrc.rows;
    // Create a destination image with 3 channels for color (CV_8UC3)
    cv::Mat imgCorrected = cv::Mat::zeros(DIMY, DIMX, CV_8UC3);

    for (int y = 0; y < DIMY; ++y) {
        for (int x = 0; x < DIMX; ++x) {
            double xd = x + 1.0, yd = y + 1.0;
            double denom = k(6) * xd + k(7) * yd + 1;
            if (std::abs(denom) < 1e-9) continue;

            double xu = (k(0) * xd + k(1) * yd + k(2)) / denom - 1;
            double yu = (k(3) * xd + k(4) * yd + k(5)) / denom - 1;
            
            int x_src = static_cast<int>(round(xu));
            int y_src = static_cast<int>(round(yu));

            if (x_src >= 0 && x_src < DIMX && y_src >= 0 && y_src < DIMY) {
                // Access pixels using cv::Vec3b for 3-channel 8-bit images
                imgCorrected.at<cv::Vec3b>(y, x) = imgSrc.at<cv::Vec3b>(y_src, x_src);
            }
        }
    }
    return imgCorrected;
}