// File: core/Analysis.hpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include "Arguments.hpp" 
#include "Math.hpp"

#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

// --- STRUCTURE DEFINITIONS ---
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

struct CurveData {
    std::string name;
    std::string camera_model;
    std::vector<double> signal_ev;
    std::vector<double> snr_db;
    cv::Mat poly_coeffs;
    std::string generated_command;
};

// --- ANALYSIS FUNCTION DECLARATIONS ---
std::string GetCameraModel(const std::string& filename);
Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points);
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k);
// MODIFIED: Renamed the last parameter for clarity
PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double patch_ratio);
std::optional<std::vector<double>> ExtractRawPixels(const std::string& filename);
double CalculateMean(const std::vector<double>& data);
double CalculateQuantile(std::vector<double>& data, double percentile);
std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream);
std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream);
std::optional<double> EstimateMeanBrightness(const std::string& filename, float sample_ratio = 0.1f);
bool PrepareAndSortFiles(ProgramOptions& opts, std::ostream& log_stream);