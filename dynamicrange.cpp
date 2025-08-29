// dynamicrange.cpp
#include "core/arguments.hpp"
#include "core/functions.hpp" // <-- Incluimos el nuevo fichero de cabecera
#include "spline.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>

#include <libraw/libraw.h>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

namespace fs = std::filesystem;

// --- FUNCIÓN PRINCIPAL ---
int main(int argc, char* argv[]) {
    
    const int NCOLS = 11;
    const int NROWS = 7;
    const double SAFE = 50.0;

    ProgramOptions opts = parse_arguments(argc, argv);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n[CONFIGURACIÓN FINAL]\n";
    std::cout << "Nivel de negro: " << opts.dark_value << "\n";
    std::cout << "Punto de saturación: " << opts.saturation_value << "\n";
    std::cout << "Fichero de salida: " << opts.output_filename << "\n\n";
    std::cout << "Iniciando proceso de cálculo de Rango Dinámico..." << std::endl;

    std::vector<std::string> filenames = opts.input_files;
    
    if (filenames.empty()){
        std::cerr << "Error: No se proporcionaron ficheros de entrada." << std::endl;
        return 1;
    }
    std::sort(filenames.begin(), filenames.end());

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
            k = calculate_keystone_params(xu, xd); // <-- Llamada a la función movida
            std::cout << "  - Parámetros Keystone calculados." << std::endl;
        }

        cv::Mat imgc = undo_keystone(imgBayer, k); // <-- Llamada a la función movida

        double xtl = (119.0 + 99.0) / 2.0; double ytl = (170.0 + 158.0) / 2.0;
        double xbr = (2515.0 + 2473.0) / 2.0; double ybr = (1687.0 + 1679.0) / 2.0;
        cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
        cv::Mat imgcrop = imgc(crop_area);

        PatchAnalysisResult patch_data = analyze_patches(imgcrop.clone(), NCOLS, NROWS, SAFE); // <-- Llamada a la función movida
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