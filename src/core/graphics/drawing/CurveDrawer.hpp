// File: src/core/graphics/drawing/CurveDrawer.hpp
/**
 * @file CurveDrawer.hpp
 * @brief Declares a component for drawing geometric data on a plot.
 * @details This module adheres to SRP by encapsulating the logic for drawing
 * the geometric shapes of a plot, specifically the fitted curves and the
 * raw data scatter points.
 */
#pragma once

#include "../RenderContext.hpp"
#include "../../analysis/Analysis.hpp"
#include <cairo/cairo.h>
#include <map>
#include <string>
#include <vector>

namespace DynaRange::Graphics::Drawing {

/**
 * @class CurveDrawer
 * @brief Responsible for drawing curves and scatter points onto a Cairo context.
 */
class CurveDrawer {
public:
    /**
     * @brief Draws the fitted polynomial curve for a single CurveData object.
     * @param cr The cairo drawing context.
     * @param curve The curve data to be drawn.
     * @param bounds A map containing the plot boundaries for coordinate mapping.
     * @param ctx The rendering context with canvas dimensions.
     * @param alpha The opacity level for the curve (0.0 to 1.0).
     */
    void Draw(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const RenderContext& ctx, double alpha) const;

    /**
     * @brief Draws the raw data scatter points for a single CurveData object.
     * @param cr The cairo drawing context.
     * @param curve The curve data containing the points to be drawn.
     * @param bounds A map containing the plot boundaries for coordinate mapping.
     * @param ctx The rendering context with canvas dimensions.
     * @param alpha The opacity level for the points (0.0 to 1.0).
     */
    void DrawPoints(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const RenderContext& ctx, double alpha) const;
};

} // namespace DynaRange::Graphics::Drawing