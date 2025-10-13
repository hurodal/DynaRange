// File: src/core/graphics/PlotBase.cpp
/**
 * @file src/core/graphics/PlotBase.cpp
 * @brief Implements the low-level Cairo drawing functions for the plot base.
 */
#include "PlotBase.hpp"
#include "drawing/AxisDrawer.hpp"
#include "drawing/GridDrawer.hpp"
#include "drawing/TitleDrawer.hpp"
#include "drawing/AxisLabelDrawer.hpp"
#include "drawing/FooterDrawer.hpp"

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void DrawPlotBase(cairo_t* cr, const DynaRange::Graphics::RenderContext& ctx, const std::string& title, const RawChannelSelection& channels, const std::map<std::string, double>& bounds,
    const std::string& command_text, const std::vector<double>& snr_thresholds)
{
    // The drawing is now a clean sequence of calls to specialized components,
    // representing the different layers of the plot.
    // 1. Draw the background, border, and axis numerical ticks.
    const DynaRange::Graphics::Drawing::AxisDrawer axis_drawer;
    axis_drawer.Draw(cr, bounds, ctx);
    
    // 2. Draw the grid and threshold lines inside the plot area.
    const DynaRange::Graphics::Drawing::GridDrawer grid_drawer;
    grid_drawer.Draw(cr, bounds, ctx, snr_thresholds);
    
    // 3. Draw high-level textual annotations.
    const DynaRange::Graphics::Drawing::TitleDrawer title_drawer;
    title_drawer.Draw(cr, ctx, title, channels);

    const DynaRange::Graphics::Drawing::AxisLabelDrawer axis_label_drawer;
    axis_label_drawer.Draw(cr, ctx);
    
    const DynaRange::Graphics::Drawing::FooterDrawer footer_drawer;
    footer_drawer.Draw(cr, ctx, command_text);
}