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

  cv::Mat raw_img = dark_file.GetRawImage();
  if (raw_img.empty())
    return std::nullopt;

  cv::Rect active_area(
      dark_file.GetLeftMargin(),
      dark_file.GetTopMargin(),
      dark_file.GetActiveWidth(),
      dark_file.GetActiveHeight()
  );

  if (active_area.width <= 0 || active_area.height <= 0 || 
      (active_area.x + active_area.width) > raw_img.cols ||
      (active_area.y + active_area.height) > raw_img.rows) {
      log_stream << _("Warning: Invalid active area defined in RAW metadata. Using global mean as fallback.") << std::endl;
      double global_mean_value = cv::mean(raw_img)[0];
      log_stream << _("Black level obtained (global mean): ") << std::fixed
                 << std::setprecision(2) << global_mean_value << std::endl;
      return global_mean_value;
  }
  
  // Se crea una copia explícita (.clone()) para asegurar que la memoria es continua.
  cv::Mat active_img = raw_img(active_area).clone();

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

  cv::Mat raw_img = sat_file.GetRawImage();
  if (raw_img.empty())
    return std::nullopt;
  
  cv::Rect active_area(
      sat_file.GetLeftMargin(),
      sat_file.GetTopMargin(),
      sat_file.GetActiveWidth(),
      sat_file.GetActiveHeight()
  );

  if (active_area.width <= 0 || active_area.height <= 0 || 
      (active_area.x + active_area.width) > raw_img.cols ||
      (active_area.y + active_area.height) > raw_img.rows) {
      log_stream << _("Warning: Invalid active area defined in RAW metadata. Using global median as fallback.") << std::endl;
      std::vector<double> pixels;
      pixels.reserve(raw_img.total());
      raw_img.reshape(1, 1).convertTo(pixels, CV_64F);
      double global_median = CalculateQuantile(pixels, 0.5);
      log_stream << _("Saturation point obtained (global median): ") << std::fixed
                 << std::setprecision(2) << global_median << std::endl;
      return global_median;
  }
  
  // Se crea una copia explícita (.clone()) para asegurar que la memoria es continua.
  cv::Mat active_img = raw_img(active_area).clone();

  std::vector<double> pixels;
  pixels.reserve(active_img.total());
  active_img.reshape(1, 1).convertTo(pixels, CV_64F);

  double median_value = CalculateQuantile(pixels, 0.5);

  log_stream << _("Saturation point obtained (active area median): ") << std::fixed
             << std::setprecision(2) << median_value << std::endl;
  return median_value;
}