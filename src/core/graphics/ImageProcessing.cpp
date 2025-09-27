// File: src/core/graphics/ImageProcessing.cpp
/**
 * @file core/ImageProcessing.cpp
 * @brief Implements geometric image processing functions.
 */
#include "ImageProcessing.hpp"
#include "../io/RawFile.hpp"
#include <libintl.h>

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
    
    // Extract a single bayer channel (e.g., Green) which is half the size.
    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    for (int r = 0; r < imgBayer.rows; ++r) {
        for (int c = 0; c < imgBayer.cols; ++c) {
            imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
        }
    }
    
    cv::Mat img_corrected = UndoKeystone(imgBayer, keystone_params);
    const auto& dst_pts = chart.GetDestinationPoints();

    // --- Active Logic: Original Simple Bounding Box Crop ---
    // This is the old logic that was previously inside AnalyzeSingleRawFile.
    // It performs a direct crop on the bounding box of the destination points
    // without any safety gap. This logic produces the original "good" results.
    double xtl = dst_pts[0].x;
    double ytl = dst_pts[0].y;
    double xbr = dst_pts[2].x;
    double ybr = dst_pts[2].y;
    cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));

    // --- Common Logic Continues ---
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
