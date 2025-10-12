// File: src/core/graphics/geometry/KeystoneCorrection.cpp
/**
 * @file KeystoneCorrection.cpp
 * @brief Implements the geometric keystone correction functions.
 */
#include "KeystoneCorrection.hpp"
#include <cmath>

namespace DynaRange::Graphics::Geometry {

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
    int DIMX = imgSrc.cols;
    int DIMY = imgSrc.rows;
    cv::Mat imgCorrected = cv::Mat::zeros(DIMY, DIMX, CV_32FC1);
    for (int y = 0; y < DIMY; ++y) {
        for (int x = 0; x < DIMX; ++x) {
            double denom = k(6) * x + k(7) * y + 1;
            if (std::abs(denom) < 1e-9) continue;
            int xu = static_cast<int>(round((k(0) * x + k(1) * y + k(2)) / denom));
            int yu = static_cast<int>(round((k(3) * x + k(4) * y + k(5)) / denom));
            if (xu >= 0 && xu < DIMX && yu >= 0 && yu < DIMY) {
                imgCorrected.at<float>(y, x) = imgSrc.at<float>(yu, xu);
            }
        }
    }
    return imgCorrected;
}

} // namespace DynaRange::Graphics::Geometry