// Fichero: core/graphics/Drawing.hpp
#pragma once

#include "../Analysis.hpp"
#include <cairo/cairo.h>
#include <string>
#include <vector>
#include <map>

// Se mueven las constantes de dimensión aquí.
constexpr int PLOT_WIDTH = 1920;
constexpr int PLOT_HEIGHT = 1080;

// Dibuja la base de la gráfica: ejes, rejilla, títulos, etc.
void DrawPlotBase(cairo_t* cr, const std::string& title, const std::map<std::string, double>& bounds);

// Dibuja las curvas de datos, puntos y etiquetas EV sobre la base de la gráfica.
void DrawCurvesAndData(cairo_t* cr, const std::vector<CurveData>& curves, const std::map<std::string, double>& bounds);