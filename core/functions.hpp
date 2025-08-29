// core/functions.hpp
#pragma once

#include <string>
#include <vector>
#include <optional>

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

// --- FUNCIONES DE CÁLCULO Y EXTRACCIÓN DE DATOS ---
std::optional<std::vector<double>> extract_raw_pixels(const std::string& filename);
double calculate_mean(const std::vector<double>& data);
double calculate_quantile(std::vector<double>& data, double percentile);

// ---  Declaraciones de las funciones de procesamiento de ficheros ---
double process_dark_frame(const std::string& filename);
double process_saturation_frame(const std::string& filename);

// --- Para hacer un análisis estádistico de la saturación de un fichero raw ---
std::optional<double> estimate_mean_brightness(const std::string& filename, float sample_ratio = 0.1f);