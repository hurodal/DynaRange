// File: src/core/graphics/drawing/GridDrawer.hpp
/**
 * @file GridDrawer.hpp
 * @brief Declares a component for drawing the grid and threshold lines of a plot.
 * @details This module adheres to SRP by encapsulating the logic for drawing all
 * lines within the plot area, including the main grid and the SNR threshold lines.
 */
#pragma once

#include "../RenderContext.hpp"
#include <cairo/cairo.h>
#include <map>
#include <string>
#include <vector>

namespace DynaRange::Graphics::Drawing {

/**
 * @class GridDrawer
 * @brief Responsible for drawing grid and threshold lines onto a Cairo context.
 */
class GridDrawer {
public:
    /**
     * @brief Draws the main grid lines and the SNR threshold lines.
     * @param cr The cairo drawing context.
     * @param bounds A map containing the plot boundaries for coordinate mapping.
     * @param ctx The rendering context with canvas dimensions.
     * @param snr_thresholds A vector of SNR thresholds to draw as horizontal dashed lines.
     */
    void Draw(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx, const std::vector<double>& snr_thresholds) const;

private:
    void DrawGridLines(cairo_t* cr, const std::map<std::string, double>& bounds, const RenderContext& ctx) const;
    void DrawThresholdLines(cairo_t* cr, const std::map<std::string, double>& bounds, const std::vector<double>& snr_thresholds, const RenderContext& ctx) const;
    void DrawDashedLine(cairo_t* cr, double x1, double y1, double x2, double y2, double dash_length) const;
};

} // namespace DynaRange::Graphics::Drawing