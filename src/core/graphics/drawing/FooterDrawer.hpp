// File: src/core/graphics/drawing/FooterDrawer.hpp
/**
 * @file FooterDrawer.hpp
 * @brief Declares a component for drawing footer elements on a plot.
 * @details This module adheres to SRP by encapsulating the logic for drawing
 * all textual elements in the footer area, such as the command line string
 * and the generation timestamp.
 */
#pragma once

#include "../RenderContext.hpp"
#include <cairo/cairo.h>
#include <string>

namespace DynaRange::Graphics::Drawing {

/**
 * @class FooterDrawer
 * @brief Responsible for drawing footer annotations like command text and timestamps.
 */
class FooterDrawer {
public:
    /**
     * @brief Draws the command text and timestamp onto a Cairo context.
     * @param cr The cairo drawing context.
     * @param ctx The rendering context with canvas dimensions.
     * @param command_text The command-line text to display.
     */
    void Draw(cairo_t* cr, const RenderContext& ctx, const std::string& command_text) const;
};

} // namespace DynaRange::Graphics::Drawing