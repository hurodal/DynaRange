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
#include "../analysis/Analysis.hpp" // For DataSource

namespace PlotColors {
    // Core Colors
    constexpr double BLACK[] = {0.0, 0.0, 0.0};
    constexpr double WHITE[] = {1.0, 1.0, 1.0};
    constexpr double RED[]   = {255.0/255.0, 0.0, 0.0}; // Vibrant red
    constexpr double BLUE[]  = {0.0, 0.0, 220.0/255.0}; // Vibrant blue
    constexpr double GREEN_LIGHT[] = {0.0, 255.0/255.0, 0.0};// A clear green for G1
    constexpr double GREEN_DARK[] = {0.0, 200.0/255.0, 0.0}; // A darker green for G2    

    // Greyscale Palette
    constexpr double GREY_10[] = {0.1, 0.1, 0.1};
    constexpr double GREY_20[] = {0.2, 0.2, 0.2};
    constexpr double GREY_50[] = {0.5, 0.5, 0.5};
    constexpr double GREY_90[] = {0.9, 0.9, 0.9};

    /**
     * @brief The percentage of opacity to decrease for each overlapping curve layer.
     * @details A value of 0.05 corresponds to a 5% decrement per layer.
     */
    constexpr double OPACITY_DECREMENT_STEP = 0.0;

    // --- HELPER FUNCTIONS FOR CAIRO ---
    inline void cairo_set_source_black(cairo_t* cr) { cairo_set_source_rgb(cr, BLACK[0], BLACK[1], BLACK[2]); }
    inline void cairo_set_source_white(cairo_t* cr) { cairo_set_source_rgb(cr, WHITE[0], WHITE[1], WHITE[2]); }
    inline void cairo_set_source_red(cairo_t* cr) { cairo_set_source_rgb(cr, RED[0], RED[1], RED[2]); }
    inline void cairo_set_source_blue(cairo_t* cr) { cairo_set_source_rgb(cr, BLUE[0], BLUE[1], BLUE[2]); }
    inline void cairo_set_source_green_light(cairo_t* cr) { cairo_set_source_rgb(cr, GREEN_LIGHT[0], GREEN_LIGHT[1], GREEN_LIGHT[2]); }
    inline void cairo_set_source_green_dark(cairo_t* cr) { cairo_set_source_rgb(cr, GREEN_DARK[0], GREEN_DARK[1], GREEN_DARK[2]); }
    inline void cairo_set_source_grey_10(cairo_t* cr) { cairo_set_source_rgb(cr, GREY_10[0], GREY_10[1], GREY_10[2]); }
    inline void cairo_set_source_grey_20(cairo_t* cr) { cairo_set_source_rgb(cr, GREY_20[0], GREY_20[1], GREY_20[2]); }
    inline void cairo_set_source_grey_50(cairo_t* cr) { cairo_set_source_rgb(cr, GREY_50[0], GREY_50[1], GREY_50[2]); }
    inline void cairo_set_source_grey_90(cairo_t* cr) { cairo_set_source_rgb(cr, GREY_90[0], GREY_90[1], GREY_90[2]); }

    // Helper to set color based on the data source channel.
    inline void SetSourceFromChannel(cairo_t* cr, DataSource channel) {
        switch (channel) {
            case DataSource::R:   cairo_set_source_red(cr); break;
            case DataSource::G1:  cairo_set_source_green_light(cr); break;
            case DataSource::G2:  cairo_set_source_green_dark(cr);  break;
            case DataSource::B:   cairo_set_source_blue(cr);        break;
            case DataSource::AVG: cairo_set_source_black(cr); break;
        }
    }
    
    // New helper to set color with an alpha channel for opacity.
    inline void SetSourceFromChannelWithAlpha(cairo_t* cr, DataSource channel, double alpha) {
        switch (channel) {
            case DataSource::R:   cairo_set_source_rgba(cr, RED[0], RED[1], RED[2], alpha); break;
            case DataSource::G1:  cairo_set_source_rgba(cr, GREEN_LIGHT[0], GREEN_LIGHT[1], GREEN_LIGHT[2], alpha); break;
            case DataSource::G2:  cairo_set_source_rgba(cr, GREEN_DARK[0], GREEN_DARK[1], GREEN_DARK[2], alpha); break;
            case DataSource::B:   cairo_set_source_rgba(cr, BLUE[0], BLUE[1], BLUE[2], alpha); break;
            case DataSource::AVG: cairo_set_source_rgba(cr, BLACK[0], BLACK[1], BLACK[2], alpha); break;
        }
    }
}