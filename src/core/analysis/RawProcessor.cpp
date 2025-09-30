// File: src/core/analysis/RawProcessor.cpp
/**
 * @file src/core/analysis/RawProcessor.cpp
 * @brief Implements processing of dark and saturation frames for sensor
 * calibration.
 */
#include "RawProcessor.hpp"
#include "../io/RawFile.hpp"
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
  if (active_img.empty())
    return std::nullopt;
  
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
  if (active_img.empty())
    return std::nullopt;
  
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

std::optional<double> OldProcessSaturationFrame(const std::string &filename,
                                                std::ostream &log_stream) {
  log_stream << _("Calculating saturation point from: ") << filename << "..."
             << std::endl;
  RawFile sat_file(filename);
  if (!sat_file.Load())
    return std::nullopt;

  int bit_depth = 0;
  if (bit_depth == 0) {
    log_stream << _("[WARNING] Could not determine bit depth from RAW "
                    "metadata. Assuming 14-bit.")
               << std::endl;
    bit_depth = 14;
  }

  cv::Mat raw_img = sat_file.GetRawImage();
  if (raw_img.empty())
    return std::nullopt;

  std::vector<double> pixels;
  pixels.reserve(raw_img.total());
  raw_img.reshape(1, 1).convertTo(pixels, CV_64F);

  // Using 5th percentile of the brightest pixels to avoid sensor defects
  double quantile_value = CalculateQuantile(pixels, 0.05);
  
  double expected_saturation = (1 << bit_depth) - 1;
  // 2^bit_depth - 1
  log_stream << _("Detected bit depth: ") << bit_depth << _(" bits")
             << std::endl;
  log_stream << _("Expected saturation level: ") << expected_saturation
             << std::endl;
  log_stream << _("Measured saturation (5th percentile): ") << std::fixed
             << std::setprecision(2) << quantile_value << std::endl;
  
  if (bit_depth == 14 && quantile_value < 10000) {
    log_stream << _("[WARNING] Measured saturation (") << quantile_value
               << _(") is much lower than expected (") << expected_saturation
               << _("). ")
               << _("This may indicate underexposure or non-linear processing.")
               << std::endl;
  }

  return quantile_value;
}