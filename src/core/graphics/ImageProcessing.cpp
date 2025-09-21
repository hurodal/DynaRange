// File: src/core/graphics/ImageProcessing.cpp
/**
 * @file core/ImageProcessing.cpp
 * @brief Implements geometric image processing functions.
 */
#include "ImageProcessing.hpp"
#include "../io/RawFile.hpp"

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

cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k) {
    int DIMX = imgSrc.cols; int DIMY = imgSrc.rows;
    cv::Mat imgCorrected = cv::Mat::zeros(DIMY, DIMX, CV_32FC1);
    for (int y = 0; y < DIMY; ++y) {
        for (int x = 0; x < DIMX; ++x) {
            double xd = x + 1.0, yd = y + 1.0;
            double denom = k(6) * xd + k(7) * yd + 1.0;
            double xu = (k(0) * xd + k(1) * yd + k(2)) / denom;
            double yu = (k(3) * xd + k(4) * yd + k(5)) / denom;
            int x_src = static_cast<int>(round(xu)) - 1;
            int y_src = static_cast<int>(round(yu)) - 1;
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
    cv::Mat img_float = raw_file.GetNormalizedImage(opts.dark_value, opts.saturation_value);
    if(img_float.empty()){
        log_stream << "Error: Could not get normalized image for: " << raw_file.GetFilename() << std::endl;
        return {};
    }
    //log_stream << "  - Info: Black=" << opts.dark_value << ", Saturation=" << opts.saturation_value << std::endl;
    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    for (int r = 0; r < imgBayer.rows; ++r) {
        for (int c = 0; c < imgBayer.cols; ++c) {
            imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
        }
    }
    
    //log_stream << "  - Applying Keystone correction..." << std::endl;
    // Se usa el parámetro pre-calculado 'keystone_params'
    cv::Mat img_corrected = UndoKeystone(imgBayer, keystone_params);
    
    const auto& dst_pts = chart.GetDestinationPoints();
    cv::Rect crop_area(round(dst_pts[0].x), round(dst_pts[0].y), round(dst_pts[2].x - dst_pts[0].x), round(dst_pts[2].y - dst_pts[0].y));
    
    if (crop_area.x < 0 || crop_area.y < 0 || crop_area.width <= 0 || crop_area.height <= 0 ||
        crop_area.x + crop_area.width > img_corrected.cols ||
        crop_area.y + crop_area.height > img_corrected.rows) {
        log_stream << "Error: Invalid crop area calculated for keystone correction." << std::endl;
        return {};
    }

    return img_corrected(crop_area);
}