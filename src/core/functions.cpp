// core/functions.cpp
#include "functions.hpp"
#include <libraw/libraw.h>
#include <vector>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

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
 * @param log_stream The output stream for progress messages.
 * @return An optional containing the calculated black level, or nullopt on failure.
 */
std::optional<double> process_dark_frame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating black level from: " << filename << "..." << std::endl;
    auto pixels_opt = extract_raw_pixels(filename);
    if (!pixels_opt) {
        return std::nullopt;
    }
    
    double mean_value = calculate_mean(*pixels_opt);
    log_stream << "[INFO] -> Black level obtained: " 
               << std::fixed << std::setprecision(2) << mean_value << std::endl;
              
    return mean_value;
}

/**
 * @brief Processes a saturation RAW file to get the saturation point (quantile).
 * @param filename Path to the saturation RAW file.
 * @param log_stream The output stream for progress messages.
 * @return An optional containing the calculated saturation point, or nullopt on failure.
 */
std::optional<double> process_saturation_frame(const std::string& filename, std::ostream& log_stream) {
    log_stream << "[INFO] Calculating saturation point from: " << filename << "..." << std::endl;
    auto pixels_opt = extract_raw_pixels(filename);
    if (!pixels_opt) {
        return std::nullopt;
    }

    double quantile_value = calculate_quantile(*pixels_opt, 0.05);
    log_stream << "[INFO] -> Saturation point obtained (5th percentile): " 
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

/**
 * @brief Pre-analyzes and sorts the input files based on their mean brightness.
 * @param opts The ProgramOptions struct, whose input_files member will be sorted in place.
 * @param log_stream The output stream for progress messages.
 * @return True if successful, false if no files could be processed.
 */
bool prepare_and_sort_files(ProgramOptions& opts, std::ostream& log_stream) {
    // Temporary structure to associate each file with its brightness.
    struct FileExposureInfo {
        std::string filename;
        double mean_brightness;
    };

    std::vector<FileExposureInfo> exposure_data;
    log_stream << "Pre-analyzing files to sort by exposure (using fast sampling)..." << std::endl;

    // Pre-analysis loop: calculates the estimated brightness of each file.
    for (const std::string& name : opts.input_files) {
        auto mean_val_opt = estimate_mean_brightness(name, 0.05f);
        if (mean_val_opt) {
            exposure_data.push_back({name, *mean_val_opt});
            log_stream << "  - " << "File: " << fs::path(name).filename().string()
                       << ", " << "Estimated brightness: " << std::fixed << std::setprecision(2) << *mean_val_opt << std::endl;
        }
    }

    if (exposure_data.empty()) {
        log_stream << "Error: None of the input files could be processed." << std::endl;
        return false;
    }

    // Sort the list of files based on mean brightness.
    std::sort(exposure_data.begin(), exposure_data.end(),
        [](const FileExposureInfo& a, const FileExposureInfo& b) {
            return a.mean_brightness < b.mean_brightness;
        }
    );

    // Update the 'opts' file list with the now-sorted list.
    opts.input_files.clear();
    for (const auto& info : exposure_data) {
        opts.input_files.push_back(info.filename);
    }
    
    log_stream << "Sorting finished. Starting Dynamic Range calculation process..." << std::endl;
    return true;
}

// Realiza un ajuste de mínimos cuadrados para encontrar los coeficientes de un polinomio.
void polyfit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order)
{
    CV_Assert(src_x.rows > 0 && src_y.rows > 0 && src_x.total() == src_y.total() && src_x.rows >= order + 1);

    cv::Mat A = cv::Mat::zeros(src_x.rows, order + 1, CV_64F);

    for (int i = 0; i < src_x.rows; ++i) {
        for (int j = 0; j <= order; ++j) {
            A.at<double>(i, j) = std::pow(src_x.at<double>(i), j);
        }
    }
    
    // Inversión de columnas para que los coeficientes salgan en el orden esperado (mayor a menor potencia)
    cv::Mat A_flipped;
    cv::flip(A, A_flipped, 1);

    cv::solve(A_flipped, src_y, dst, cv::DECOMP_SVD);
}

// --- FUNCIÓN COMPLETA Y CORREGIDA ---

void generate_snr_plot(
    const std::string& output_filename,
    const std::string& image_title,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs)
{
    // --- 1. Configuración del Lienzo y Dimensiones ---
    const int width = 1920, height = 1080;
    const int margin_left = 120, margin_bottom = 100, margin_top = 80, margin_right = 60;
    const int plot_area_width = width - margin_left - margin_right;
    const int plot_area_height = height - margin_top - margin_bottom;

    cv::Mat plot_img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

    // --- 2. Calcular Límites de los Datos y rangos de los ejes ---
    double min_ev_data = signal_ev.empty() ? 0 : *std::min_element(signal_ev.begin(), signal_ev.end());
    double max_ev_data = signal_ev.empty() ? 0 : *std::max_element(signal_ev.begin(), signal_ev.end());
    double min_db_data = snr_db.empty() ? -5 : *std::min_element(snr_db.begin(), snr_db.end());
    double max_db_data = snr_db.empty() ? 25 : *std::max_element(snr_db.begin(), snr_db.end());
    double ev_range = max_ev_data - min_ev_data;
    double db_range = max_db_data - min_db_data;
    double ev_pad = std::max(1.0, ev_range * 0.1);
    double db_pad = std::max(5.0, db_range * 0.1);
    double min_ev = floor(min_ev_data - ev_pad);
    double max_ev = ceil(max_ev_data + ev_pad);
    double min_db = floor(min_db_data - db_pad);
    double max_db = ceil(max_db_data + db_pad);
    min_db = std::min({min_db, 0.0, 12.0});
    max_db = std::max({max_db, 0.0, 12.0});
    min_db = floor(min_db / 5.0) * 5.0;
    max_db = ceil(max_db / 5.0) * 5.0;
    min_ev = floor(min_ev);
    max_ev = ceil(max_ev);

    auto map_coords = [&](double ev, double db) {
        int px = static_cast<int>(margin_left + (ev - min_ev) / (max_ev - min_ev) * plot_area_width);
        int py = static_cast<int>((height - margin_bottom) - (db - min_db) / (max_db - min_db) * plot_area_height);
        return cv::Point(px, py);
    };

    // --- 3. Dibujar Grid y Etiquetas de Ejes ---
    cv::Rect plot_rect(margin_left, margin_top, plot_area_width, plot_area_height);
    cv::rectangle(plot_img, plot_rect, cv::Scalar(0,0,0), 2);
    for (double ev = min_ev; ev <= max_ev; ev += 1.0) { cv::line(plot_img, map_coords(ev, min_db), map_coords(ev, max_db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)ev), {map_coords(ev, min_db).x-10, height-margin_bottom+25}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    for (double db = min_db; db <= max_db; db += 5.0) { cv::line(plot_img, map_coords(min_ev, db), map_coords(max_ev, db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)db), {margin_left-40, map_coords(min_ev, db).y+7}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    cv::putText(plot_img, "RAW Exposure (EV)", {width/2-70, height-25}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {0,0,0}, 2);
    
    // CORREGIDO: La imagen temporal AHORA se crea con el mismo fondo blanco explícito.
    cv::Mat y_label_img(plot_area_height, 40, CV_8UC3, cv::Scalar(255, 255, 255));
    
    cv::putText(y_label_img, "SNR (dB)", cv::Point(5, plot_area_height / 2 + 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,0,0), 2);
    cv::rotate(y_label_img, y_label_img, cv::ROTATE_90_COUNTERCLOCKWISE);
    y_label_img.copyTo(plot_img(cv::Rect(20, margin_top + plot_area_height / 2 - (y_label_img.rows / 2), y_label_img.cols, y_label_img.rows)));
    cv::putText(plot_img, "SNR Curve - " + image_title, {width/2-150, margin_top-30}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);

    // --- 4. Dibujar Líneas de Referencia ---
    cv::line(plot_img, map_coords(min_ev, 12.0), map_coords(max_ev, 12.0), cv::Scalar(0, 100, 0), 2, cv::LINE_AA);
    cv::putText(plot_img, "12 dB (Photographic DR)", {map_coords(max_ev, 12.0).x - 250, map_coords(max_ev, 12.0).y - 10}, cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 100, 0), 1);
    cv::line(plot_img, map_coords(min_ev, 0.0), map_coords(max_ev, 0.0), cv::Scalar(150, 0, 0), 2, cv::LINE_AA);
    cv::putText(plot_img, "0 dB (Engineering DR)", {map_coords(max_ev, 0.0).x - 220, map_coords(max_ev, 0.0).y - 10}, cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(150, 0, 0), 1);

    // --- 5. Dibujar Curva Polinómica ---
    std::vector<cv::Point> poly_points;
    for (int px = margin_left; px < width - margin_right; ++px) {
        double ev = min_ev + (double)(px - margin_left) / plot_area_width * (max_ev - min_ev);
        double snr_poly = 0.0;
        for (int i = 0; i < poly_coeffs.rows; ++i) { snr_poly += poly_coeffs.at<double>(i) * std::pow(ev, poly_coeffs.rows - 1 - i); }
        poly_points.push_back(map_coords(ev, snr_poly));
    }
    cv::polylines(plot_img, poly_points, false, cv::Scalar(0, 0, 200), 3, cv::LINE_AA);

    // --- 6. Dibujar Puntos ---
    for (size_t i = 0; i < signal_ev.size(); ++i) {
        cv::circle(plot_img, map_coords(signal_ev[i], snr_db[i]), 5, cv::Scalar(255, 0, 0), -1, cv::LINE_AA);
        cv::circle(plot_img, map_coords(signal_ev[i], snr_db[i]), 5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    }

    // --- 7. Guardar la Imagen ---
    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Plot saved to: " << output_filename << std::endl;
}

// --- FUNCIÓN COMPLETA Y CORREGIDA ---

void generate_summary_plot(
    const std::string& output_filename,
    const std::vector<CurveData>& all_curves)
{
    // --- 1. Configuración del Lienzo ---
    const int width = 1920, height = 1080;
    const int margin_left = 120, margin_bottom = 100, margin_top = 120, margin_right = 300;
    const int plot_area_width = width - margin_left - margin_right;
    const int plot_area_height = height - margin_top - margin_bottom;
    
    // CORREGIDO: Usamos el método explícito para crear el fondo blanco.
    cv::Mat plot_img(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

    // --- 2. Calcular Límites Globales ---
    double min_ev = 1e6, max_ev = -1e6, min_db = 1e6, max_db = -1e6;
    if (all_curves.empty()) return;
    for (const auto& curve : all_curves) {
        if (!curve.signal_ev.empty()) {
            min_ev = std::min(min_ev, *std::min_element(curve.signal_ev.begin(), curve.signal_ev.end()));
            max_ev = std::max(max_ev, *std::max_element(curve.signal_ev.begin(), curve.signal_ev.end()));
        }
        if (!curve.snr_db.empty()) {
            min_db = std::min(min_db, *std::min_element(curve.snr_db.begin(), curve.snr_db.end()));
            max_db = std::max(max_db, *std::max_element(curve.snr_db.begin(), curve.snr_db.end()));
        }
    }
    min_db = floor(min_db / 5.0) * 5.0 - 5.0;
    max_db = ceil(max_db / 5.0) * 5.0 + 5.0;
    min_ev = floor(min_ev) - 1.0;
    max_ev = ceil(max_ev) + 1.0;

    auto map_coords = [&](double ev, double db) {
        int px = static_cast<int>(margin_left + (ev - min_ev) / (max_ev - min_ev) * plot_area_width);
        int py = static_cast<int>((height - margin_bottom) - (db - min_db) / (max_db - min_db) * plot_area_height);
        return cv::Point(px, py);
    };

    // --- 3. Dibujar Grid, Ejes y Título ---
    cv::rectangle(plot_img, {margin_left, margin_top}, {width - margin_right, height - margin_bottom}, {0,0,0}, 2);
    for (double ev = ceil(min_ev); ev <= floor(max_ev); ev += 1.0) { cv::line(plot_img, map_coords(ev, min_db), map_coords(ev, max_db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)ev), {map_coords(ev, min_db).x-10, height-margin_bottom+25}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    for (double db = ceil(min_db); db <= floor(max_db); db += 5.0) { cv::line(plot_img, map_coords(min_ev, db), map_coords(max_ev, db), {220,220,220}, 1); cv::putText(plot_img, std::to_string((int)db), {margin_left-40, map_coords(min_ev, db).y+7}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1); }
    cv::putText(plot_img, "RAW Exposure (EV)", {width/2-70, height-25}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {0,0,0}, 2);
    
    cv::Mat y_label_img(plot_area_height, 40, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::putText(y_label_img, "SNR (dB)", cv::Point(5, plot_area_height / 2 + 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,0,0), 2);
    cv::rotate(y_label_img, y_label_img, cv::ROTATE_90_COUNTERCLOCKWISE);
    y_label_img.copyTo(plot_img(cv::Rect(20, margin_top + plot_area_height / 2 - (y_label_img.rows / 2), y_label_img.cols, y_label_img.rows)));
    cv::putText(plot_img, "SNR Curves Summary", {width/2-150, margin_top-30}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0,0,0}, 2);

    // --- 4. Definir Paleta de Colores para las Curvas ---
    std::vector<cv::Scalar> colors = { {200,0,0}, {0,0,200}, {0,150,0}, {0,150,150}, {150,150,0}, {150,0,150}, {0,75,150}, {100,100,100}, {200,100,0}, {0,100,200}, {100,200,0}, {100,0,200} };

    // --- 5. Dibujar cada Curva y su Leyenda ---
    int legend_y_pos = margin_top;
    for (size_t i = 0; i < all_curves.size(); ++i) {
        const auto& curve = all_curves[i];
        cv::Scalar color = colors[i % colors.size()];
        std::vector<cv::Point> poly_points;
        for (double ev = min_ev; ev <= max_ev; ev += 0.1) {
            double snr_poly = 0.0;
            for (int j = 0; j < curve.poly_coeffs.rows; ++j) { snr_poly += curve.poly_coeffs.at<double>(j) * std::pow(ev, curve.poly_coeffs.rows - 1 - j); }
            poly_points.push_back(map_coords(ev, snr_poly));
        }
        cv::polylines(plot_img, poly_points, false, color, 2, cv::LINE_AA);
        cv::rectangle(plot_img, {width - margin_right + 10, legend_y_pos - 12}, {width - margin_right + 30, legend_y_pos + 8}, color, -1);
        cv::putText(plot_img, fs::path(curve.name).stem().string(), {width - margin_right + 40, legend_y_pos + 5}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0,0,0}, 1);
        legend_y_pos += 25;
    }

    // --- 6. Guardar la Imagen ---
    cv::imwrite(output_filename, plot_img);
    std::cout << "[INFO] Summary Plot saved to: " << output_filename << std::endl;
}