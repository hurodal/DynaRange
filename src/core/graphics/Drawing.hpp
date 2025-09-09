// File: core/graphics/Drawing.hpp
#pragma once

#include "../Analysis.hpp"
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>

constexpr int PLOT_WIDTH = 1920;
constexpr int PLOT_HEIGHT = 1080;

// Added snr_thresholds to draw them dynamically
void DrawPlotBase(cairo_t* cr, const std::string& title, const std::map<std::string, double>& bounds, const std::string& command_text, const std::vector<double>& snr_thresholds);

void DrawCurvesAndData(cairo_t* cr, const std::vector<CurveData>& curves, const std::map<std::string, double>& bounds);