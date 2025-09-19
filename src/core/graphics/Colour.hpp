// File: src/core/graphics/Colour.hpp
/**
 * @file src/core/graphics/Colour.hpp
 * @brief Declares a semantic palette of named colors for consistent plotting.
 *
 * Colors are defined as RGB triplets (0.0 to 1.0) using universal, semantic names.
 * This allows developers to choose colors based on their visual property,
 * not their usage context (e.g., use 'BLACK', not 'GRID_LINE_COLOR').
 * The context is documented in the source code where the color is used.
 *
 * Provides helper functions to easily set these colors in Cairo contexts.
 */
#pragma once
#include <cairo/cairo.h>

namespace PlotColors {
    // Core Colors
    constexpr double BLACK[] = {0.0, 0.0, 0.0};           // Pure black
    constexpr double WHITE[] = {1.0, 1.0, 1.0};           // Pure white
    constexpr double RED[]   = {200.0/255.0, 0.0, 0.0};   // Vibrant red
    constexpr double BLUE[]  = {0.0, 0.0, 200.0/255.0};   // Vibrant blue

    // Greyscale Palette
    constexpr double GREY_10[] = {0.1, 0.1, 0.1};         // Very dark gray
    constexpr double GREY_20[] = {0.2, 0.2, 0.2};         // Dark gray (used for grid lines and borders)
    constexpr double GREY_50[] = {0.5, 0.5, 0.5};         // Medium gray
    constexpr double GREY_90[] = {0.9, 0.9, 0.9};         // Light gray (background)

    // --- HELPER FUNCTIONS FOR CAIRO ---
    // Inline functions to set color from semantic name directly
    // These are automatically inlined by the compiler — zero runtime cost.

    inline void cairo_set_source_black(cairo_t* cr) {
        cairo_set_source_rgb(cr, BLACK[0], BLACK[1], BLACK[2]);
    }

    inline void cairo_set_source_white(cairo_t* cr) {
        cairo_set_source_rgb(cr, WHITE[0], WHITE[1], WHITE[2]);
    }

    inline void cairo_set_source_red(cairo_t* cr) {
        cairo_set_source_rgb(cr, RED[0], RED[1], RED[2]);
    }

    inline void cairo_set_source_blue(cairo_t* cr) {
        cairo_set_source_rgb(cr, BLUE[0], BLUE[1], BLUE[2]);
    }

    inline void cairo_set_source_grey_10(cairo_t* cr) {
        cairo_set_source_rgb(cr, GREY_10[0], GREY_10[1], GREY_10[2]);
    }

    inline void cairo_set_source_grey_20(cairo_t* cr) {
        cairo_set_source_rgb(cr, GREY_20[0], GREY_20[1], GREY_20[2]);
    }

    inline void cairo_set_source_grey_50(cairo_t* cr) {
        cairo_set_source_rgb(cr, GREY_50[0], GREY_50[1], GREY_50[2]);
    }

    inline void cairo_set_source_grey_90(cairo_t* cr) {
        cairo_set_source_rgb(cr, GREY_90[0], GREY_90[1], GREY_90[2]);
    }
}