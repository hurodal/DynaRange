// core/functions.hpp
#pragma once

#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

// Estructuras de datos que se usan tanto en main como en las funciones
struct DynamicRangeResult {
    std::string filename;
    double dr_12db;
    double dr_0db;
    int patches_used;
};

struct PatchAnalysisResult {
    std::vector<double> signal;
    std::vector<double> noise;
    cv::Mat image_with_patches;
};

// Declaraciones de las funciones de procesamiento
Eigen::VectorXd calculate_keystone_params(
    const std::vector<cv::Point2d>& src_points,
    const std::vector<cv::Point2d>& dst_points
);

cv::Mat undo_keystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k);

PatchAnalysisResult analyze_patches(cv::Mat imgcrop, int NCOLS, int NROWS, double SAFE);