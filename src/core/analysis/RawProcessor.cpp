// File: src/core/analysis/RawProcessor.cpp
/**
 * @file src/core/analysis/RawProcessor.cpp
 * @brief Implements processing of dark and saturation frames for sensor
 * calibration.
 */
#include "RawProcessor.hpp"
#include "../io/raw/RawFile.hpp"
#include "../math/Math.hpp" // Para CalculateQuantile (ahora no usada) y CalculateMean (si existe)
#include <iomanip>
#include <iostream>
#include <libintl.h>
#include <opencv2/core.hpp> // *** NUEVO INCLUDE para cv::mean ***
#include <numeric>         // *** NUEVO INCLUDE para std::accumulate (si CalculateMean no existe) ***

#define _(string) gettext(string)

std::optional<double> ProcessDarkFrame(const std::string &filename,
                                       std::ostream &log_stream) {
  log_stream << _("Calculating black level from: ") << filename << "..."
             << std::endl;
  RawFile dark_file(filename);
  if (!dark_file.Load())
    return std::nullopt;

  // Get the already-cropped active image directly.
  cv::Mat active_img = dark_file.GetActiveRawImage();
  if (active_img.empty()) {
    log_stream << _("[FATAL ERROR] Could not read direct raw sensor data from file: ") << filename << std::endl;
    log_stream << _("  This is likely because the file is in a compressed RAW format (e.g., from a smartphone) that is not supported for calibration.") << std::endl;
    return std::nullopt;
  }

  // --- *** INICIO: CAMBIO DE MEDIANA A MEDIA *** ---
  // Convertir la imagen a CV_64F para cálculo preciso de la media
  cv::Mat active_img_double;
  active_img.convertTo(active_img_double, CV_64F);

  // Calcular la media de los píxeles del área activa
  cv::Scalar mean_scalar = cv::mean(active_img_double);
  double mean_value = mean_scalar[0]; // cv::mean devuelve un Scalar, tomamos el primer (y único) canal

  /* --- CÓDIGO ANTERIOR (MEDIANA) COMENTADO ---
  // Convert the active area to a vector to calculate the median.
  std::vector<double> pixels;
  pixels.reserve(active_img.total());
  active_img.reshape(1, 1).convertTo(pixels, CV_64F);

  // Calculate the median (quantile 0.5) on the active pixels.
  // This is robust against hot pixels and correctly identifies a true zero black level.
  double median_value = CalculateQuantile(pixels, 0.5);
  if (median_value == 0.0 && cv::countNonZero(active_img) > 0) {
      log_stream << _("[INFO] Zero black level detected, ignoring hot pixels.") << std::endl;
  }
  log_stream << _("Black level obtained (active area median): ") << std::fixed
             << std::setprecision(2) << median_value << std::endl;
  return median_value;
  --- FIN CÓDIGO ANTERIOR --- */

  // Nueva salida de log indicando que se usa la media
  log_stream << _("Black level obtained (active area mean): ") << std::fixed
             << std::setprecision(2) << mean_value << std::endl;
  return mean_value; // Devolver la media calculada
  // --- *** FIN: CAMBIO DE MEDIANA A MEDIA *** ---
}

// La función ProcessSaturationFrame no necesita cambios, ya usa mediana como R.
std::optional<double> ProcessSaturationFrame(const std::string &filename,
                                             std::ostream &log_stream) {
  log_stream << _("Calculating saturation point from: ") << filename << "..."
             << std::endl;
  RawFile sat_file(filename);
  if (!sat_file.Load())
    return std::nullopt;

  // Get the already-cropped active image directly.
  cv::Mat active_img = sat_file.GetActiveRawImage();
  if (active_img.empty()) {
    log_stream << _("[FATAL ERROR] Could not read direct raw sensor data from file: ") << filename << std::endl;
    log_stream << _("  This is likely because the file is in a compressed RAW format (e.g., from a smartphone) that is not supported for calibration.") << std::endl;
    return std::nullopt;
  }

  // Convert only the active area to a vector for quantile calculation.
  std::vector<double> pixels;
  pixels.reserve(active_img.total());
  active_img.reshape(1, 1).convertTo(pixels, CV_64F);

  // Calculate the median (quantile 0.5) on the active pixels only.
  // This is how R code doit.
  double median_value = CalculateQuantile(pixels, 0.5);
  log_stream << _("Saturation point obtained (active area median): ") << std::fixed
             << std::setprecision(2) << median_value << std::endl;
  return median_value;
}