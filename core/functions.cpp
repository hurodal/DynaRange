// core/functions.cpp
#include "functions.hpp"
#include <libraw/libraw.h>
#include <vector>
#include <numeric>   // Para std::accumulate
#include <algorithm> // Para std::nth_element
#include <iostream>
#include <iomanip>   // Para std::fixed y std::setprecision

/**
 * @brief Calcula los parámetros de una transformación proyectiva (keystone).
 * @param src_points Vector con los 4 puntos de origen (esquinas del objeto distorsionado).
 * @param dst_points Vector con los 4 puntos de destino (esquinas del rectángulo deseado).
 * @return Un objeto Eigen::VectorXd con los 8 parámetros de la transformación.
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
 * @brief Aplica una corrección de distorsión keystone a una imagen.
 * @param imgSrc Imagen de entrada (debe ser de tipo CV_32FC1).
 * @param k Parámetros de la transformación obtenidos de calculate_keystone_params.
 * @return Una nueva imagen cv::Mat con la corrección aplicada.
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
 * @brief Analiza una imagen dividiéndola en parches y calcula la señal y el ruido de cada uno.
 * @param imgcrop Imagen recortada a analizar.
 * @param NCOLS Número de columnas en la rejilla de parches.
 * @param NROWS Número de filas en la rejilla de parches.
 * @param SAFE Margen de seguridad para evitar los bordes de cada parche.
 * @return Una estructura PatchAnalysisResult con los vectores de señal/ruido y una imagen visual.
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
 * @brief Extrae todos los valores de píxeles de un fichero RAW a un vector de dobles.
 * @param filename Ruta al fichero RAW.
 * @return Un std::optional que contiene un std::vector<double> con los datos si tiene éxito,
 * o std::nullopt si hay un error.
 */
std::optional<std::vector<double>> extract_raw_pixels(const std::string& filename) {
    LibRaw raw_processor;
    if (raw_processor.open_file(filename.c_str()) != LIBRAW_SUCCESS) {
        std::cerr << "Error: No se pudo abrir el fichero RAW: " << filename << std::endl;
        return std::nullopt;
    }
    if (raw_processor.unpack() != LIBRAW_SUCCESS) {
        std::cerr << "Error: No se pudieron decodificar los datos RAW de: " << filename << std::endl;
        return std::nullopt;
    }

    int width = raw_processor.imgdata.sizes.raw_width;
    int height = raw_processor.imgdata.sizes.raw_height;
    size_t num_pixels = (size_t)width * height;

    if (num_pixels == 0) {
        return std::nullopt;
    }

    std::vector<double> pixels;
    pixels.reserve(num_pixels); // Reservamos memoria para eficiencia

    unsigned short* raw_data = raw_processor.imgdata.rawdata.raw_image;
    for (size_t i = 0; i < num_pixels; ++i) {
        pixels.push_back(static_cast<double>(raw_data[i]));
    }

    return pixels;
}

/**
 * @brief Calcula la media (promedio) de los valores en un vector.
 * @param data Vector de entrada (constante, no se modifica).
 * @return El valor medio como un double.
 */
double calculate_mean(const std::vector<double>& data) {
    if (data.empty()) {
        return 0.0;
    }
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

/**
 * @brief Calcula un cuantil (percentil) específico de un conjunto de datos.
 * @param data Vector de entrada. IMPORTANTE: El contenido del vector será modificado (parcialmente ordenado).
 * @param percentile El percentil deseado (ej. 0.05 para el 5%, 0.5 para la mediana).
 * @return El valor del cuantil como un double.
 */
double calculate_quantile(std::vector<double>& data, double percentile) {
    if (data.empty()) {
        return 0.0;
    }
    
    // Calculamos el índice del elemento deseado
    size_t n = static_cast<size_t>(data.size() * percentile);
    // Aseguramos que el índice está dentro de los límites
    n = std::min(n, data.size() - 1);

    // std::nth_element reorganiza el vector de forma que el elemento en la posición 'n'
    // es el que estaría en esa posición si el vector estuviera completamente ordenado.
    // Todos los elementos a su izquierda son menores o iguales. Es mucho más rápido que un sort completo.
    std::nth_element(data.begin(), data.begin() + n, data.end());
    
    return data[n];
}

/**
 * @brief Procesa un fichero RAW de dark frame para obtener el nivel de negro (media).
 * @param filename Ruta al fichero RAW del dark frame.
 * @return El nivel de negro calculado. Termina el programa si hay un error.
 */
double process_dark_frame(const std::string& filename) {
    std::cout << "[INFO] Calculando nivel de negro desde: " << filename << "..." << std::endl;
    auto pixels_opt = extract_raw_pixels(filename);
    if (!pixels_opt) {
        std::cerr << "Error fatal: No se pudo procesar el dark frame. Saliendo." << std::endl;
        exit(1);
    }
    
    // Calculamos, guardamos, imprimimos y devolvemos el valor.
    double mean_value = calculate_mean(*pixels_opt);
    std::cout << "[INFO] -> Nivel de negro obtenido: " 
              << std::fixed << std::setprecision(2) << mean_value << std::endl;
              
    return mean_value;
}

/**
 * @brief Procesa un fichero RAW de saturación para obtener el punto de saturación (cuantil).
 * @param filename Ruta al fichero RAW de saturación.
 * @return El punto de saturación calculado. Termina el programa si hay un error.
 */
double process_saturation_frame(const std::string& filename) {
    std::cout << "[INFO] Calculando punto de saturación desde: " << filename << "..." << std::endl;
    auto pixels_opt = extract_raw_pixels(filename);
    if (!pixels_opt) {
        std::cerr << "Error fatal: No se pudo procesar el fichero de saturación. Saliendo." << std::endl;
        exit(1);
    }

    // Calculamos, guardamos, imprimimos y devolvemos el valor.
    double quantile_value = calculate_quantile(*pixels_opt, 0.05);
    std::cout << "[INFO] -> Punto de saturación obtenido (percentil 5%): " 
              << std::fixed << std::setprecision(2) << quantile_value << std::endl;

    return quantile_value;
}

/**
 * @brief Estima el brillo medio de un fichero RAW leyendo solo una fracción de sus píxeles.
 * @param filename Ruta al fichero RAW.
 * @param sample_ratio Fracción de píxeles a muestrear (ej. 0.1 para el 10%).
 * @return Un std::optional que contiene la media estimada si tiene éxito.
 */
std::optional<double> estimate_mean_brightness(const std::string& filename, float sample_ratio) {
    LibRaw raw_processor;
    if (raw_processor.open_file(filename.c_str()) != LIBRAW_SUCCESS || raw_processor.unpack() != LIBRAW_SUCCESS) {
        // No mostramos error aquí, ya que extract_raw_pixels lo hará si falla más tarde
        return std::nullopt;
    }

    size_t num_pixels = (size_t)raw_processor.imgdata.sizes.raw_width * raw_processor.imgdata.sizes.raw_height;
    if (num_pixels == 0) {
        return std::nullopt;
    }

    // Calculamos el "paso" para leer 1 de cada N píxeles.
    // Si sample_ratio es 0.1 (10%), el paso será 10.
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