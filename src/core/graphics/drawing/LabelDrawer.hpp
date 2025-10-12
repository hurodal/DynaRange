// File: src/core/graphics/drawing/LabelDrawer.hpp
/**
 * @file LabelDrawer.hpp
 * @brief Declares a component for drawing textual labels on a plot.
 * @details This module adheres to SRP by encapsulating the logic for drawing
 * all textual elements on the plot data area, such as ISO labels and the
 * complex dynamic range value labels at threshold intersections.
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
 * @class LabelDrawer
 * @brief Responsible for drawing all textual labels onto a Cairo context.
 */
class LabelDrawer {
public:
    /**
     * @brief Draws all labels, including curve (ISO) and threshold intersection (DR) labels.
     * @param cr The cairo drawing context.
     * @param curves The vector of all curve data.
     * @param results The vector of all dynamic range results.
     * @param bounds A map containing the plot boundaries for coordinate mapping.
     * @param ctx The rendering context with canvas dimensions.
     */
    void Draw(cairo_t* cr, const std::vector<CurveData>& curves, const std::vector<DynamicRangeResult>& results, const std::map<std::string, double>& bounds, const RenderContext& ctx) const;

private:
    void DrawCurveLabel(cairo_t* cr, const CurveData& curve, const std::map<std::string, double>& bounds, const RenderContext& ctx) const;

    void DrawThresholdIntersection(cairo_t* cr, const std::string& text_to_draw, DataSource channel, double primary_px, double primary_py, double primary_angle_rad, int channel_index, int group_size, const RenderContext& ctx) const;
};

} // namespace DynaRange::Graphics::Drawing