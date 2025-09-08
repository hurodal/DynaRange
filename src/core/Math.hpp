// Fichero: core/Math.hpp
#pragma once

#include <opencv2/core.hpp>
#include <vector>
#include <optional>

// Realiza un ajuste polinómico a un conjunto de puntos 2D.
void PolyFit(const cv::Mat& src_x, const cv::Mat& src_y, cv::Mat& dst, int order);

// Encuentra la intersección de una curva polinómica (SNR = f(EV)) con un valor SNR objetivo.
std::optional<double> FindIntersectionEV(const cv::Mat& coeffs, double target_snr_db, double min_ev, double max_ev);