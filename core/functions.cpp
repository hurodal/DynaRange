// core/functions.cpp
#include "functions.hpp"
#include <libraw/libraw.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <iomanip>

/**
 * @brief Calculates the parameters of a projective (keystone) transformation.
 * @param src_points Vector with the 4 source points (corners of the distorted object).
 * @param dst_points Vector with the 4 destination points (corners of the desired rectangle).
 * @return An Eigen::VectorXd object with the 8 transformation parameters.
 */
Eigen::VectorXd calculate_keystone_params(
    const std::vector<cv::Point2d>& src_points,
    const std::vector<cv::Point2d>& dst_points
) {
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

/**
 * @brief Applies a keystone distortion correction to an image.
 * @param imgSrc Input image (must be of type CV_32FC1).
 * @param k Transformation parameters obtained from calculate_keystone_params.
 * @return A new cv::Mat image with the correction applied.
 */
cv::Mat undo_keystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k) {
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

/**
 * @brief Analyzes an image by dividing it into patches and calculates the signal and noise for each.
 * @param imgcrop Cropped image to be analyzed.
 * @param NCOLS Number of columns in the patch grid.
 * @param NROWS Number of rows in the patch grid.
 * @param SAFE Safety margin to avoid the edges of each patch.
 * @return A PatchAnalysisResult structure with the signal/noise vectors and a visual image.
 */
PatchAnalysisResult analyze_patches(cv::Mat imgcrop, int NCOLS, int NROWS, double SAFE) {
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

/**
 * @brief Extracts all pixel values from a RAW file into a vector of doubles.
 * @param filename Path to the RAW file.
 * @return An std::optional containing a std::vector<double> with the data on success,
 * or std::nullopt on error.
 */
std::optional<std::vector<double>> extract_raw_pixels(const std::string& filename) {
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

    if (num_pixels == 0) {
        return std::nullopt;
    }

    std::vector<double> pixels;
    pixels.reserve(num_pixels);

    unsigned short* raw_data = raw_processor.imgdata.rawdata.raw_image;
    for (size_t i = 0; i < num_pixels; ++i) {
        pixels.push_back(static_cast<double>(raw_data[i]));
    }

    return pixels;
}

/**
 * @brief Calculates the mean (average) of the values in a vector.
 * @param data Input vector (const, not modified).
 * @return The mean value as a double.
 */
double calculate_mean(const std::vector<double>& data) {
    if (data.empty()) {
        return 0.0;
    }
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

/**
 * @brief Calculates a specific quantile (percentile) of a dataset.
 * @param data Input vector. IMPORTANT: The contents of the vector will be modified (partially sorted).
 * @param percentile The desired percentile (e.g., 0.05 for 5%, 0.5 for the median).
 * @return The quantile value as a double.
 */
double calculate_quantile(std::vector<double>& data, double percentile) {
    if (data.empty()) {
        return 0.0;
    }
    
    size_t n = static_cast<size_t>(data.size() * percentile);
    n = std::min(n, data.size() - 1);

    std::nth_element(data.begin(), data.begin() + n, data.end());
    
    return data[n];
}

/**
 * @brief Processes a dark frame RAW file to get the black level (mean).
 * @param filename Path to the dark frame RAW file.
 * @return An optional containing the calculated black level, or nullopt on failure.
 */
std::optional<double> process_dark_frame(const std::string& filename) {
    std::cout << "[INFO] Calculating black level from: " << filename << "..." << std::endl;
    auto pixels_opt = extract_raw_pixels(filename);
    if (!pixels_opt) {
        return std::nullopt;
    }
    
    double mean_value = calculate_mean(*pixels_opt);
    std::cout << "[INFO] -> Black level obtained: " 
              << std::fixed << std::setprecision(2) << mean_value << std::endl;
              
    return mean_value;
}

/**
 * @brief Processes a saturation RAW file to get the saturation point (quantile).
 * @param filename Path to the saturation RAW file.
 * @return An optional containing the calculated saturation point, or nullopt on failure.
 */
std::optional<double> process_saturation_frame(const std::string& filename) {
    std::cout << "[INFO] Calculating saturation point from: " << filename << "..." << std::endl;
    auto pixels_opt = extract_raw_pixels(filename);
    if (!pixels_opt) {
        return std::nullopt;
    }

    double quantile_value = calculate_quantile(*pixels_opt, 0.05);
    std::cout << "[INFO] -> Saturation point obtained (5th percentile): " 
              << std::fixed << std::setprecision(2) << quantile_value << std::endl;

    return quantile_value;
}

/**
 * @brief Estimates the mean brightness of a RAW file by reading only a fraction of its pixels.
 * @param filename Path to the RAW file.
 * @param sample_ratio Fraction of pixels to sample (e.g., 0.1 for 10%). The default value is specified in the .hpp.
 * @return An std::optional containing the estimated mean on success.
 */
std::optional<double> estimate_mean_brightness(const std::string& filename, float sample_ratio) {
    LibRaw raw_processor;
    if (raw_processor.open_file(filename.c_str()) != LIBRAW_SUCCESS || raw_processor.unpack() != LIBRAW_SUCCESS) {
        return std::nullopt;
    }

    size_t num_pixels = (size_t)raw_processor.imgdata.sizes.raw_width * raw_processor.imgdata.sizes.raw_height;
    if (num_pixels == 0) {
        return std::nullopt;
    }

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