// File: src/core/graphics/RenderContext.hpp
/**
 * @file RenderContext.hpp
 * @brief Defines a context structure for plot rendering operations.
 * @details This struct encapsulates geometric properties like canvas dimensions,
 * allowing drawing functions to be resolution-agnostic.
 */
#pragma once

namespace DynaRange::Graphics {

    /**
     * @struct RenderContext
     * @brief Holds geometric information for a single plot rendering operation.
     */
    struct RenderContext {
        /// The total width of the canvas to draw on.
        int base_width;
        
        /// The total height of the canvas to draw on.
        int base_height;
    };

} // namespace DynaRange::Graphics