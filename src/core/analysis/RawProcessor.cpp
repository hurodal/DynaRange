// File: src/core/analysis/RawProcessor.cpp
/**
 * @file src/core/analysis/RawProcessor.cpp
 * @brief Implements processing of dark and saturation frames for sensor
 * calibration.
 */
#include "RawProcessor.hpp"
#include "../io/raw/RawFile.hpp"
#include "../math/Math.hpp"
#include <iomanip>
#include <iostream>
#include <libintl.h>

#define _(string) gettext(string)

/**
 * @file src/core/analysis/RawProcessor.cpp
 * @brief Implements processing of dark and saturation frames for sensor
 * calibration.
 */
#include "RawProcessor.hpp"
#include "../io/raw/RawFile.hpp"
#include "../math/Math.hpp"
#include <iomanip>
#include <iostream>
#include <libintl.h>

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
  
  // Calculate the mean on the active area only.
  double mean_value = cv::mean(active_img)[0];
  log_stream << _("Black level obtained (active area mean): ") << std::fixed
             << std::setprecision(2) << mean_value << std::endl;
  return mean_value;
}

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