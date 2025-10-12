// File: src/core/graphics/geometry/KeystoneCorrection.cpp
/**
 * @file KeystoneCorrection.cpp
 * @brief Implements the geometric keystone correction functions.
 */
#include "KeystoneCorrection.hpp"
#include <cmath>
#include <opencv2/core.hpp>

namespace DynaRange::Graphics::Geometry {

cv::Mat CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points) {
    cv::Mat A = cv::Mat::zeros(8, 8, CV_64F);
    cv::Mat b = cv::Mat::zeros(8, 1, CV_64F);
    
    for (int i = 0; i < 4; ++i) {
        const auto& xu = src_points[i].x;
        const auto& yu = src_points[i].y;
        const auto& xd = dst_points[i].x; 
        const auto& yd = dst_points[i].y;

        A.at<double>(2 * i, 0) = xd;
        A.at<double>(2 * i, 1) = yd;
        A.at<double>(2 * i, 2) = 1;
        A.at<double>(2 * i, 3) = 0;
        A.at<double>(2 * i, 4) = 0;
        A.at<double>(2 * i, 5) = 0;
        A.at<double>(2 * i, 6) = -xd * xu;
        A.at<double>(2 * i, 7) = -yd * xu;

        A.at<double>(2 * i + 1, 0) = 0;
        A.at<double>(2 * i + 1, 1) = 0;
        A.at<double>(2 * i + 1, 2) = 0;
        A.at<double>(2 * i + 1, 3) = xd;
        A.at<double>(2 * i + 1, 4) = yd;
        A.at<double>(2 * i + 1, 5) = 1;
        A.at<double>(2 * i + 1, 6) = -xd * yu;
        A.at<double>(2 * i + 1, 7) = -yd * yu;

        b.at<double>(2 * i) = xu;
        b.at<double>(2 * i + 1) = yu;
    }

    cv::Mat k;
    cv::solve(A, b, k, cv::DECOMP_SVD);
    return k;
}

cv::Mat UndoKeystone(const cv::Mat& imgSrc, const cv::Mat& k) {
    int DIMX = imgSrc.cols;
    int DIMY = imgSrc.rows;
    cv::Mat imgCorrected = cv::Mat::zeros(DIMY, DIMX, CV_32FC1);
    for (int y = 0; y < DIMY; ++y) {
        for (int x = 0; x < DIMX; ++x) {
            double denom = k.at<double>(6) * x + k.at<double>(7) * y + 1;
            if (std::abs(denom) < 1e-9) continue;
            int xu = static_cast<int>(round((k.at<double>(0) * x + k.at<double>(1) * y + k.at<double>(2)) / denom));
            int yu = static_cast<int>(round((k.at<double>(3) * x + k.at<double>(4) * y + k.at<double>(5)) / denom));
            if (xu >= 0 && xu < DIMX && yu >= 0 && yu < DIMY) {
                imgCorrected.at<float>(y, x) = imgSrc.at<float>(yu, xu);
            }
        }
    }
    return imgCorrected;
}

} // namespace DynaRange::Graphics::Geometry