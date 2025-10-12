// File: src/core/graphics/drawing/TitleDrawer.cpp
/**
 * @file TitleDrawer.cpp
 * @brief Implements the drawing logic for the plot title and subtitle.
 */
#include "TitleDrawer.hpp"
#include "../FontManager.hpp"
#include "../Colour.hpp"
#include "../PlotBase.hpp" // For MARGIN_TOP
#include <libintl.h>

#define _(string) gettext(string)

namespace DynaRange::Graphics::Drawing {

void TitleDrawer::Draw(cairo_t* cr, const RenderContext& ctx, const std::string& title, const ProgramOptions& opts) const
{
    const FontManager font_manager(ctx);
    cairo_text_extents_t extents;

    // --- Main Title ---
    PlotColors::cairo_set_source_black(cr);
    font_manager.SetTitleFont(cr);
    cairo_text_extents(cr, title.c_str(), &extents);
    double current_x = ctx.base_width / 2.0 - extents.width / 2.0;
    double current_y = MARGIN_TOP - 40;
    cairo_move_to(cr, current_x, current_y);
    cairo_show_text(cr, title.c_str());

    // --- Channel Subtitle ---
    font_manager.SetSubtitleFont(cr);
    current_x += extents.x_advance + 10;
    const auto& channels = opts.raw_channels;
    bool has_avg = channels.AVG;
    bool has_r = channels.R, has_g1 = channels.G1, has_g2 = channels.G2, has_b = channels.B;
    bool has_channels = has_r || has_g1 || has_g2 || has_b;
    
    if (has_avg && !has_channels) {
        PlotColors::cairo_set_source_grey_50(cr);
        std::string avg_text = _("(Average channels)");
        cairo_move_to(cr, current_x, current_y);
        cairo_show_text(cr, avg_text.c_str());
    } else if (has_channels) {
        PlotColors::cairo_set_source_grey_50(cr);
        std::string prefix = " (";
        if (has_avg) {
            prefix += _("Average & ");
        }
        prefix += _("Channels -> ");

        cairo_move_to(cr, current_x, current_y);
        cairo_show_text(cr, prefix.c_str());
        cairo_text_extents(cr, prefix.c_str(), &extents);
        current_x += extents.x_advance;

        auto draw_channel_name = 
        [&](const std::string& name, DataSource channel) {
            cairo_move_to(cr, current_x, current_y);
            PlotColors::SetSourceFromChannel(cr, channel);
            cairo_show_text(cr, name.c_str());
            cairo_text_extents(cr, name.c_str(), &extents);
            current_x += extents.x_advance;
        };

        if (has_r) draw_channel_name("R ", DataSource::R);
        if (has_g1) draw_channel_name("G1 ", DataSource::G1);
        if (has_g2) draw_channel_name("G2 ", DataSource::G2);
        if (has_b) draw_channel_name("B ", DataSource::B);

        PlotColors::cairo_set_source_grey_50(cr);
        cairo_move_to(cr, current_x, current_y);
        cairo_show_text(cr, ")");
    }
}

} // namespace DynaRange::Graphics::Drawing