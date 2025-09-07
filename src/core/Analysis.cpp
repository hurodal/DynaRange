// Fichero: core/Analysis.cpp
#include "Analysis.hpp"
#include <libraw/libraw.h>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

std::string GetCameraModel(const std::string& filename) {
    LibRaw raw_processor;
    if (raw_processor.open_file(filename.c_str()) != LIBRAW_SUCCESS) {
        // No se puede abrir, devuelve cadena vacía
        return "";
    }
    // Devuelve el modelo de la cámara. Si no existe, idata.model es una cadena vacía.
    return std::string(raw_processor.imgdata.idata.model);
}

Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points) {
    Eigen::Matrix<double, 8, 8> A;
    Eigen::Vector<double, 8> b;
    for (int i = 0; i < 4; ++i) {
        const auto& xu = src_points[i].x; const auto& yu = src_points[i].y;
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

PatchAnalysisResult AnalyzePatches(cv::Mat imgcrop, int NCOLS, int NROWS, double SAFE) {
    std::vector<double> signal_vec, noise_vec;
    for (int j = 0; j < NROWS; ++j) {
        for (int i = 0; i < NCOLS; ++i) {
            int x1 = round((double)i * imgcrop.cols / NCOLS + SAFE);
            int x2 = round((double)(i + 1) * imgcrop.cols / NCOLS - SAFE);
            int y1 = round((double)j * imgcrop.rows / NROWS + SAFE);
            int y2 = round((double)(j + 1) * imgcrop.rows / NROWS - SAFE);
            if (x1 >= x2 || y1 >= y2) continue;
            cv::Mat patch = imgcrop(cv::Rect(x1, y1, x2 - x1, y2 - y1));
            cv::Scalar mean, stddev;
            cv::meanStdDev(patch, mean, stddev);
            double S = mean[0], N = stddev[0];
            int sat_count = cv::countNonZero(patch > 0.9);
            double sat_ratio = (double)sat_count / (patch.rows * patch.cols);
            if (S > 0 && N > 0 && 20 * log10(S / N) >= -10 && sat_ratio < 0.01) {
                signal_vec.push_back(S); noise_vec.push_back(N);
                cv::rectangle(imgcrop, {x1, y1}, {x2, y2}, cv::Scalar(0.0), 1);
                cv::rectangle(imgcrop, {x1 - 1, y1 - 1}, {x2 + 1, y2 + 1}, cv::Scalar(1.0), 1);
            }
        }
    }
    return {signal_vec, noise_vec, imgcrop};
}

std::optional<std::vector<double>> ExtractRawPixels(const std::string& filename) {
    LibRaw raw_processor;
    if (raw_processor.open_file(filename.c_str()) != LIBRAW_SUCCESS) {
        std::cerr << "Error: Could not open RAW file: " << filename << std::endl;
        return std::nullopt;
    }
    if (raw_processor.unpack() != LIBRAW_SUCCESS) {
        std::cerr << "Error: Could not decode RAW data from: " << filename << std::endl;
        return std::nullopt;
    }
    int width = raw_processor.imgdata.sizes.raw_width;
    int height = raw_processor.imgdata.sizes.raw_height;
    size_t num_pixels = (size_t)width * height;
    if (num_pixels == 0) return std::nullopt;
    std::vector<double> pixels;
    pixels.reserve(num_pixels);
    unsigned short* raw_data = raw_processor.imgdata.rawdata.raw_image;
    for (size_t i = 0; i < num_pixels; ++i) {
        pixels.push_back(static_cast<double>(raw_data[i]));
    }
    return pixels;
}

double CalculateMean(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double CalculateQuantile(std::vector<double>& data, double percentile) {
    if (data.empty()) return 0.0;
    size_t n = static_cast<size_t>(data.size() * percentile);
    n = std::min(n, data.size() - 1);
    std::nth_element(data.begin(), data.begin() + n, data.end());
    return data[n];
}

std::optional<double> ProcessDarkFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating black level from: " << filename << "..." << std::endl;
    auto pixels_opt = ExtractRawPixels(filename);
    if (!pixels_opt) return std::nullopt;
    double mean_value = CalculateMean(*pixels_opt);
    log_stream << "[INFO] -> Black level obtained: " << std::fixed << std::setprecision(2) << mean_value << std::endl;
    return mean_value;
}

std::optional<double> ProcessSaturationFrame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating saturation point from: " << filename << "..." << std::endl;
    auto pixels_opt = ExtractRawPixels(filename);
    if (!pixels_opt) return std::nullopt;
    double quantile_value = CalculateQuantile(*pixels_opt, 0.05);
    log_stream << "[INFO] -> Saturation point obtained (5th percentile): " << std::fixed << std::setprecision(2) << quantile_value << std::endl;
    return quantile_value;
}

std::optional<double> EstimateMeanBrightness(const std::string& filename, float sample_ratio) {
    LibRaw raw_processor;
    if (raw_processor.open_file(filename.c_str()) != LIBRAW_SUCCESS || raw_processor.unpack() != LIBRAW_SUCCESS) return std::nullopt;
    size_t num_pixels = (size_t)raw_processor.imgdata.sizes.raw_width * raw_processor.imgdata.sizes.raw_height;
    if (num_pixels == 0) return std::nullopt;
    int step = (sample_ratio > 0 && sample_ratio < 1) ? static_cast<int>(1.0f / sample_ratio) : 1;
    unsigned short* raw_data = raw_processor.imgdata.rawdata.raw_image;
    double sum = 0.0;
    long long count = 0;
    for (size_t i = 0; i < num_pixels; i += step) {
        sum += static_cast<double>(raw_data[i]);
        count++;
    }
    return (count > 0) ? (sum / count) : 0.0;
}

bool PrepareAndSortFiles(ProgramOptions& opts, std::ostream& log_stream) {
    struct FileExposureInfo {
        std::string filename;
        double mean_brightness;
    };
    std::vector<FileExposureInfo> exposure_data;
    log_stream << "Pre-analyzing files to sort by exposure (using fast sampling)..." << std::endl;
    for (const std::string& name : opts.input_files) {
        auto mean_val_opt = EstimateMeanBrightness(name, 0.05f);
        if (mean_val_opt) {
            exposure_data.push_back({name, *mean_val_opt});
            log_stream << "  - " << "File: " << fs::path(name).filename().string() << ", " << "Estimated brightness: " << std::fixed << std::setprecision(2) << *mean_val_opt << std::endl;
        }
    }
    if (exposure_data.empty()) {
        log_stream << "Error: None of the input files could be processed." << std::endl;
        return false;
    }
    std::sort(exposure_data.begin(), exposure_data.end(), [](const FileExposureInfo& a, const FileExposureInfo& b) {
        return a.mean_brightness < b.mean_brightness;
    });
    opts.input_files.clear();
    for (const auto& info : exposure_data) {
        opts.input_files.push_back(info.filename);
    }
    log_stream << "Sorting finished. Starting Dynamic Range calculation process..." << std::endl;
    return true;
}

void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order) {
    CV_Assert(src_x.rows > 0 && src_y.rows > 0 && src_x.total() == src_y.total() && src_x.rows >= order + 1);
    cv::Mat A = cv::Mat::zeros(src_x.rows, order + 1, CV_64F);
    for (int i = 0; i < src_x.rows; ++i) {
        for (int j = 0; j <= order; ++j) {
            A.at<double>(i, j) = std::pow(src_x.at<double>(i), j);
        }
    }
    cv::Mat A_flipped;
    cv::flip(A, A_flipped, 1);
    cv::solve(A_flipped, src_y, dst, cv::DECOMP_SVD);
}