// File: src/core/graphics/drawing/AxisDrawer.hpp
/**
 * @file AxisDrawer.hpp
 * @brief Declares a component for drawing the plot axes, border, and background.
 * @details This module adheres to SRP by encapsulating the logic for drawing
 * the plot's static frame, including the background, border, and the numerical
 * tick labels for both axes.
 */
#pragma once

#include "../RenderContext.hpp"
#include <cairo/cairo.h>
#include <map>
#include <string>

namespace DynaRange::Graphics::Drawing {

/**
 * @class AxisDrawer
 * @brief Responsible for drawing the plot background, border, and axis tick labels.
 */
class AxisDrawer {
public:
    /**
     * @brief Draws the background, border, and axis tick labels.
     * @param cr The cairo drawing context.
     * @param bounds A map containing the plot boundaries for coordinate mapping.
     * @param ctx The rendering context with canvas dimensions.
     */
    void Draw(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const;

private:
    void DrawPlotBackgroundAndBorder(cairo_t* cr, const RenderContext& ctx) const;
    void DrawXAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const;
    void DrawYAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const;
};

} // namespace DynaRange::Graphics::Drawing