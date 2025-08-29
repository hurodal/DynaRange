// dynamicrange.cpp
// Fichero principal del ejecutable.
// Orquesta el flujo del programa: parseo de argumentos, ordenación de ficheros,
// procesamiento en bucle y guardado de resultados.

#include "core/arguments.hpp" // Para el parseo de argumentos de línea de comandos.
#include "core/functions.hpp" // Para todas las funciones de procesamiento de datos e imágenes.
#include "spline.h"           // Para la interpolación con splines.

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <fstream>

#include <libraw/libraw.h>     // Para la lectura de ficheros RAW.
#include <opencv2/opencv.hpp>  // Para el manejo de imágenes.
#include <Eigen/Dense>         // Para álgebra lineal (corrección keystone).

namespace fs = std::filesystem;

// --- FUNCIÓN PRINCIPAL ---
int main(int argc, char* argv[]) {
    
    // --- 0. CONFIGURACIÓN INICIAL ---
    // Constantes para el análisis de la carta de color.
    const int NCOLS = 11;
    const int NROWS = 7;
    const double SAFE = 50.0;

    // --- 1. PARSEO DE ARGUMENTOS ---
    // Delega toda la lógica de la línea de comandos al módulo de argumentos.
    // La función maneja errores y la petición de ayuda (--help) internamente.
    ProgramOptions opts = parse_arguments(argc, argv);

    // Muestra la configuración final que se usará en el procesamiento.
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n[CONFIGURACIÓN FINAL]\n";
    std::cout << "Nivel de negro: " << opts.dark_value << "\n";
    std::cout << "Punto de saturación: " << opts.saturation_value << "\n";
    std::cout << "Fichero de salida: " << opts.output_filename << "\n\n";

    // --- 2. PRE-ANÁLISIS Y ORDENACIÓN POR EXPOSICIÓN ---
    // Para procesar los ficheros en orden de menor a mayor exposición sin depender
    // del nombre, primero estimamos el brillo medio de cada uno usando un muestreo rápido.
    
    // Estructura temporal para asociar cada fichero con su brillo.
    struct FileExposureInfo {
        std::string filename;
        double mean_brightness;
    };

    std::vector<FileExposureInfo> exposure_data;
    std::cout << "Pre-analizando ficheros para ordenarlos por exposición (usando muestreo rápido)..." << std::endl;

    // Bucle de pre-análisis: calcula el brillo estimado de cada fichero.
    for (const std::string& name : opts.input_files) {
        // Usamos la función optimizada que solo lee una muestra de píxeles (ej. 5%).
        auto mean_val_opt = estimate_mean_brightness(name, 0.05f); 

        if (mean_val_opt) {
            exposure_data.push_back({name, *mean_val_opt});
            std::cout << "  - Fichero: " << fs::path(name).filename().string() 
                      << ", Brillo estimado: " << std::fixed << std::setprecision(2) << *mean_val_opt << std::endl;
        }
    }

    // Ordena la lista de ficheros basándose en el brillo medio, de menor a mayor.
    std::sort(exposure_data.begin(), exposure_data.end(), 
        [](const FileExposureInfo& a, const FileExposureInfo& b) {
            return a.mean_brightness < b.mean_brightness;
        }
    );

    // Crea la lista final de ficheros, ahora sí, ordenada correctamente.
    std::vector<std::string> filenames;
    for (const auto& info : exposure_data) {
        filenames.push_back(info.filename);
    }

    if (filenames.empty()){
        std::cerr << "Error: Ninguno de los ficheros de entrada pudo ser procesado." << std::endl;
        return 1;
    }

    std::cout << "Ordenación finalizada. Iniciando proceso de cálculo de Rango Dinámico..." << std::endl;
    
    // --- 3. BUCLE PRINCIPAL DE PROCESAMIENTO ---
    std::vector<DynamicRangeResult> all_results;
    Eigen::VectorXd k; // Vector para los parámetros de corrección keystone.

    for (int i = 0; i < filenames.size(); ++i) {
        const std::string& name = filenames[i];
        std::cout << "\nProcesando \"" << name << "\"..." << std::endl;

        // Apertura y decodificación del fichero RAW
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

        // Conversión de datos RAW a una matriz de OpenCV y normalización
        cv::Mat raw_image(height, width, CV_16U, raw_processor.imgdata.rawdata.raw_image);
        cv::Mat img_float;
        raw_image.convertTo(img_float, CV_32F);
        img_float = (img_float - black_level) / (sat_level - black_level);

        // Extracción de un solo canal del patrón Bayer
        cv::Mat imgBayer(height / 2, width / 2, CV_32FC1);
        for (int r = 0; r < imgBayer.rows; ++r) {
            for (int c = 0; c < imgBayer.cols; ++c) {
                imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
            }
        }

        // Si es la primera imagen, calcula los parámetros de corrección de perspectiva.
        if (i == 0) {
            std::vector<cv::Point2d> xu = {{119, 170}, {99, 1687}, {2515, 1679}, {2473, 158}};
            double xtl = (xu[0].x + xu[1].x) / 2.0; double ytl = (xu[0].y + xu[3].y) / 2.0;
            double xbr = (xu[2].x + xu[3].x) / 2.0; double ybr = (xu[1].y + xu[2].y) / 2.0;
            std::vector<cv::Point2d> xd = {{xtl, ytl}, {xtl, ybr}, {xbr, ybr}, {xbr, ytl}};
            k = calculate_keystone_params(xu, xd);
            std::cout << "  - Parámetros Keystone calculados." << std::endl;
        }

        // Aplica la corrección, recorta la imagen y analiza los parches
        cv::Mat imgc = undo_keystone(imgBayer, k);
        double xtl = (119.0 + 99.0) / 2.0; double ytl = (170.0 + 158.0) / 2.0;
        double xbr = (2515.0 + 2473.0) / 2.0; double ybr = (1687.0 + 1679.0) / 2.0;
        cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
        cv::Mat imgcrop = imgc(crop_area);
        PatchAnalysisResult patch_data = analyze_patches(imgcrop.clone(), NCOLS, NROWS, SAFE);
        
        if (patch_data.signal.empty()) {
            std::cerr << "Advertencia: No se encontraron parches válidos para " << name << std::endl;
            continue;
        }

        // Ordena los datos de los parches por su relación señal/ruido (SNR)
        std::vector<double> Signal = patch_data.signal;
        std::vector<double> Noise = patch_data.noise;
        std::vector<size_t> p(Signal.size());
        std::iota(p.begin(), p.end(), 0);
        std::sort(p.begin(), p.end(), [&](size_t i, size_t j){ return (Signal[i]/Noise[i]) < (Signal[j]/Noise[j]); });
        
        std::vector<double> sorted_Signal(Signal.size()), sorted_Noise(Signal.size());
        for(size_t idx = 0; idx < Signal.size(); ++idx) {
            sorted_Signal[idx] = Signal[p[idx]]; 
            sorted_Noise[idx] = Noise[p[idx]];
        }
        Signal = sorted_Signal;
        Noise = sorted_Noise;

        // Convierte señal y ruido a las unidades deseadas (EV y dB)
        std::vector<double> snr_db, signal_ev;
        for (size_t j = 0; j < Signal.size(); ++j) {
            snr_db.push_back(20 * log10(Signal[j] / Noise[j]));
            signal_ev.push_back(log2(Signal[j]));
        }

        // Ajusta una curva spline a los datos y calcula los puntos de DR
        tk::spline s;
        s.set_points(snr_db, signal_ev);
        double dr_12db = -s(12.0); // DR con umbral de calidad "excelente" (12 dB)
        double dr_0db = -s(0.0);   // DR con umbral de "límite aceptable" (0 dB)
        
        all_results.push_back({name, dr_12db, dr_0db, (int)Signal.size()});
    }

    // --- 4. PRESENTACIÓN Y GUARDADO DE RESULTADOS ---
    std::cout << "\n--- Resultados de Rango Dinámico ---\n";
    std::cout << std::left << std::setw(35) << "Archivo RAW"
              << std::setw(15) << "DR (12dB)" << std::setw(15) << "DR (0dB)"
              << "Parches" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    for (const auto& res : all_results) {
        std::cout << std::left << std::setw(35) << fs::path(res.filename).filename().string()
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_12db
                  << std::fixed << std::setprecision(4) << std::setw(15) << res.dr_0db
                  << res.patches_used << std::endl;
    }
    
    // Guarda los resultados en el fichero CSV especificado.
    std::ofstream csv_file(opts.output_filename);
    csv_file << "raw_file,DR_EV_12dB,DR_EV_0dB,patches_used\n";
    for (const auto& res : all_results) {
        csv_file << fs::path(res.filename).filename().string() << "," << res.dr_12db << "," << res.dr_0db << "," << res.patches_used << "\n";
    }
    csv_file.close();
    std::cout << "\nResultados guardados en " << opts.output_filename << std::endl;

    return 0;
}