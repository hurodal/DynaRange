// File: src/core/graphics/FontManager.hpp
/**
 * @file FontManager.hpp
 * @brief Declares a manager for handling dynamic font sizing in plots.
 * @details This class calculates appropriately scaled font sizes based on a
 * render context, ensuring text is proportional regardless of the canvas size.
 */
#pragma once

#include "RenderContext.hpp"
#include <cairo/cairo.h>

namespace DynaRange::Graphics {

    /**
     * @class FontManager
     * @brief Manages font styles and sizes for a given rendering context.
     */
    class FontManager {
    public:
        /**
         * @brief Constructs a FontManager for a specific rendering context.
         * @param ctx The context containing the target canvas dimensions.
         */
        explicit FontManager(const RenderContext& ctx);

        /**
         * @brief Calculates a scaled size for lengths, offsets, or fonts.
         * @param base_size The value designed for the reference width.
         * @return The calculated value for the current context's width.
         */
        double calculateScaledSize(double base_size) const;

        // --- Font Style Setters ---
        void SetTitleFont(cairo_t* cr) const;
        void SetSubtitleFont(cairo_t* cr) const;
        void SetAxisLabelFont(cairo_t* cr) const;
        void SetAxisTickFont(cairo_t* cr) const;
        void SetThresholdLabelFont(cairo_t* cr) const;
        void SetCurveLabelFont(cairo_t* cr) const;
        void SetDrValueFont(cairo_t* cr) const;
        void SetCommandFont(cairo_t* cr) const;
        void SetTimestampFont(cairo_t* cr) const;
        void SetInfoBoxFont(cairo_t* cr) const;

    private:
        /// The rendering context for which to calculate font sizes.
        const RenderContext& m_ctx;
    };

} // namespace DynaRange::Graphics