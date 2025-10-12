// File: src/core/graphics/drawing/TitleDrawer.hpp
/**
 * @file TitleDrawer.hpp
 * @brief Declares a component for drawing the main title and subtitle of a plot.
 * @details This module adheres to SRP by encapsulating the logic for drawing
 * the main plot title and the dynamically generated channel subtitle.
 */
#pragma once

#include "../RenderContext.hpp"
#include "../../arguments/ArgumentsOptions.hpp"
#include <cairo/cairo.h>
#include <string>

namespace DynaRange::Graphics::Drawing {

/**
 * @class TitleDrawer
 * @brief Responsible for drawing the plot title and subtitle.
 */
class TitleDrawer {
public:
    /**
     * @brief Draws the main title and channel subtitle onto a Cairo context.
     * @param cr The cairo drawing context.
     * @param ctx The rendering context with canvas dimensions.
     * @param title The main title of the plot.
     * @param opts The program options, used to determine the channel subtitle.
     */
    void Draw(cairo_t* cr, const RenderContext& ctx, const std::string& title, const ProgramOptions& opts) const;
};

} // namespace DynaRange::Graphics::Drawing