// File: src/core/graphics/drawing/AxisLabelDrawer.hpp
/**
 * @file AxisLabelDrawer.hpp
 * @brief Declares a component for drawing the axis labels of a plot.
 * @details This module adheres to SRP by encapsulating the logic for drawing
 * the X and Y axis titles, including text rotation for the Y-axis.
 */
#pragma once

#include "../RenderContext.hpp"
#include <cairo/cairo.h>

namespace DynaRange::Graphics::Drawing {

/**
 * @class AxisLabelDrawer
 * @brief Responsible for drawing the X and Y axis labels.
 */
class AxisLabelDrawer {
public:
    /**
     * @brief Draws the X and Y axis labels onto a Cairo context.
     * @param cr The cairo drawing context.
     * @param ctx The rendering context with canvas dimensions.
     */
    void Draw(cairo_t* cr, const RenderContext& ctx) const;
};

} // namespace DynaRange::Graphics::Drawing