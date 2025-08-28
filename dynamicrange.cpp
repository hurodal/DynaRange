// dynamicrange.cpp
// Versión que lee archivos RAW directamente usando la librería LibRaw.
//
// DEPENDENCIAS:
// - LibRaw (para leer archivos RAW)
// - OpenCV (para procesamiento de imágenes y matrices)
// - Eigen (para álgebra lineal, inversión de matrices)
// - tk::spline (para la interpolación con splines)
// - CLI11 (para el parseo de argumentos)

#include "core/arguments.hpp"  // Para parse_arguments y ProgramOptions
#include <iostream>
#include <iomanip>             // para std::setprecision
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <CLI/CLI.hpp>         // Necesario para el try-catch de errores de parseo

// LibRaw para leer archivos RAW
#include <libraw/libraw.h>

// OpenCV para manejo de imágenes
#include <opencv2/opencv.hpp>

// Eigen para álgebra lineal
#include <Eigen/Dense>

// tk::spline para interpolación
#include "spline.h"

namespace fs = std::filesystem;

// --- ESTRUCTURAS DE DATOS ---
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

// --- FUNCIONES DE PROCESAMIENTO ---
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

// --- FUNCIÓN PRINCIPAL ---
int main(int argc, char* argv[]) {
    
    // 0. CONFIGURACIÓN INICIAL
    const int NCOLS = 11;
    const int NROWS = 7;
    const double SAFE = 50.0;

    // --- Parseo de argumentos simplificado ---
    // La función parse_arguments ahora maneja la ayuda y los errores por sí misma.
    // Si hay un error o se pide ayuda, el programa terminará dentro de esa función.
    ProgramOptions opts = parse_arguments(argc, argv);

    // Mostrar configuración obtenida
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n[CONFIGURACIÓN FINAL]\n";
    std::cout << "Nivel de negro: " << opts.dark_value << "\n";
    std::cout << "Punto de saturación: " << opts.saturation_value << "\n";
    std::cout << "Fichero de salida: " << opts.output_filename << "\n\n";
 
    // La lista de ficheros ahora viene directamente de los argumentos parseados.
    std::vector<std::string> filenames = opts.input_files;
    if (filenames.empty()){
        // Este error ya no debería ocurrir si el argumento es obligatorio
        std::cerr << "Error: No se proporcionaron ficheros de entrada." << std::endl;
        return 1;
    }
    // Ordenar los ficheros por nombre
    std::sort(filenames.begin(), filenames.end());
    
    std::cout << "Iniciando proceso de cálculo de Rango Dinámico desde archivos RAW..." << std::endl;

    // 1. OBTENER LISTA DE ARCHIVOS RAW
    std::string filepath = ".";
    std::vector<std::string> raw_extensions = {".dng", ".cr2", ".cr3", ".nef", ".arw", ".orf", ".raf"};
    for (const auto& entry : fs::directory_iterator(filepath)) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (std::find(raw_extensions.begin(), raw_extensions.end(), ext) != raw_extensions.end()) {
            filenames.push_back(entry.path().string());
        }
    }
    std::sort(filenames.begin(), filenames.end());
    
    if (filenames.empty()){
        std::cerr << "Error: No se encontraron archivos RAW en el directorio actual." << std::endl;
        return 1;
    }

    // Variables para almacenar resultados
    std::vector<DynamicRangeResult> all_results;
    Eigen::VectorXd k;

    // BUCLE PRINCIPAL: PROCESAR CADA IMAGEN
    for (int i = 0; i < filenames.size(); ++i) {
        const std::string& name = filenames[i];
        std::cout << "Procesando \"" << name << "\"..." << std::endl;

        LibRaw raw_processor;
        if (raw_processor.open_file(name.c_str()) != LIBRAW_SUCCESS) {
            std::cerr << "Error: No se pudo abrir el archivo RAW: " << name << std::endl;
            continue;
        }
        if (raw_processor.unpack() != LIBRAW_SUCCESS) {
            std::cerr << "Error: No se pudieron decodificar los datos RAW de: " << name << std::endl;
            continue;
        }

        const double black_level = opts.dark_value;
        const double sat_level = opts.saturation_value;

        int width = raw_processor.imgdata.sizes.raw_width;
        int height = raw_processor.imgdata.sizes.raw_height;

        std::cout << "  - Info: Black=" << black_level << ", Saturation=" << sat_level << std::endl;

        cv::Mat raw_image(height, width, CV_16U, raw_processor.imgdata.rawdata.raw_image);
        cv::Mat img_float;
        raw_image.convertTo(img_float, CV_32F);

        img_float = (img_float - black_level) / (sat_level - black_level);

        // 2. EXTRAER CANAL BAYER Y CORREGIR KEYSTONE
        cv::Mat imgBayer(height / 2, width / 2, CV_32FC1);
        for (int r = 0; r < imgBayer.rows; ++r) {
            for (int c = 0; c < imgBayer.cols; ++c) {
                imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
            }
        }

        if (i == 0) {
            std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
            double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
            double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
            std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
            k = calculate_keystone_params(xu, xd);
            std::cout << "  - Parámetros Keystone calculados." << std::endl;
        }

        cv::Mat imgc = undo_keystone(imgBayer, k);

        // 3. RECORTAR Y ANALIZAR PARCHES
        double xtl = (119.0 + 99.0) / 2.0; double ytl = (170.0 + 158.0) / 2.0;
        double xbr = (2515.0 + 2473.0) / 2.0; double ybr = (1687.0 + 1679.0) / 2.0;
        cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
        cv::Mat imgcrop = imgc(crop_area);

        PatchAnalysisResult patch_data = analyze_patches(imgcrop.clone(), NCOLS, NROWS, SAFE);
        std::vector<double> Signal = patch_data.signal, Noise = patch_data.noise;

        if (Signal.empty()) {
            std::cerr << "Advertencia: No se encontraron parches válidos para " << name << std::endl;
            continue;
        }

        std::vector<double> SNR_vals; std::vector<size_t> p(Signal.size());
        std::iota(p.begin(), p.end(), 0);
        for(size_t idx = 0; idx < Signal.size(); ++idx) SNR_vals.push_back(Signal[idx] / Noise[idx]);
        std::sort(p.begin(), p.end(), [&](size_t i, size_t j){ return SNR_vals[i] < SNR_vals[j]; });
        std::vector<double> sorted_Signal(Signal.size()), sorted_Noise(Signal.size());
        for(size_t idx = 0; idx < Signal.size(); ++idx) {
            sorted_Signal[idx] = Signal[p[idx]]; sorted_Noise[idx] = Noise[p[idx]];
        }
        Signal = sorted_Signal; Noise = sorted_Noise;

        // 4. AJUSTE CON SPLINES Y CÁLCULO DEL DR
        std::vector<double> snr_db, signal_ev;
        for (size_t j = 0; j < Signal.size(); ++j) {
            snr_db.push_back(20 * log10(Signal[j] / Noise[j]));
            signal_ev.push_back(log2(Signal[j]));
        }

        tk::spline s;
        s.set_points(snr_db, signal_ev);
        double dr_12db = -s(12.0);
        double dr_0db = -s(0.0);
        all_results.push_back({name, dr_12db, dr_0db, (int)Signal.size()});
    }

    // 5. MOSTRAR RESULTADOS FINALES
    std::cout << "\n--- Resultados de Rango Dinámico ---\n";
    std::cout << std::left << std::setw(30) << "Archivo RAW"
              << std::setw(15) << "DR (12dB)" << std::setw(15) << "DR (0dB)"
              << "Parches" << std::endl;
    std::cout << std::string(70, '-') << std::endl;

    std::ofstream csv_file(opts.output_filename);
    csv_file << "raw_file,DR_EV_12dB,DR_EV_0dB,patches_used\n";
    for (const auto& res : all_results) {
        std::cout << std::left << std::setw(30) << res.filename
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_12db
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_0db
                  << res.patches_used << std::endl;
        csv_file << res.filename << "," << res.dr_12db << "," << res.dr_0db << "," << res.patches_used << "\n";
    }
    csv_file.close();
    std::cout << "\nResultados guardados en " << opts.output_filename << std::endl;

    return 0;
}
