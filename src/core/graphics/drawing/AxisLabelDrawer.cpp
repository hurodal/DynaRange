// File: src/core/graphics/drawing/AxisLabelDrawer.cpp
/**
 * @file AxisLabelDrawer.cpp
 * @brief Implements the drawing logic for plot axis labels.
 */
#include "AxisLabelDrawer.hpp"
#include "../FontManager.hpp"
#include "../Colour.hpp"
#include "../PlotBase.hpp" // For margins
#include <libintl.h>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace DynaRange::Graphics::Drawing {

void AxisLabelDrawer::Draw(cairo_t* cr, const RenderContext& ctx) const
{
    const FontManager font_manager(ctx);
    cairo_text_extents_t extents;

    PlotColors::cairo_set_source_black(cr);
    font_manager.SetAxisLabelFont(cr);

    // --- X-Axis Label ---
    std::string x_label = _("RAW exposure (EV)");
    cairo_text_extents(cr, x_label.c_str(), &extents);
    const double vertical_offset = font_manager.calculateScaledSize(60.0);
    cairo_move_to(cr, ctx.base_width / 2.0 - extents.width / 2.0, ctx.base_height - MARGIN_BOTTOM + vertical_offset);
    cairo_show_text(cr, x_label.c_str());

    // --- Y-Axis Label ---
    std::string y_label = _("SNR (dB)");
    cairo_text_extents(cr, y_label.c_str(), &extents);
    cairo_save(cr);
    cairo_move_to(cr, MARGIN_LEFT / 2.0 - extents.height / 2.0, ctx.base_height / 2.0 + extents.width / 2.0);
    cairo_rotate(cr, -M_PI / 2.0);
    cairo_show_text(cr, y_label.c_str());
    cairo_restore(cr);
}

} // namespace DynaRange::Graphics::Drawing