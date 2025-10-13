// File: src/core/graphics/drawing/TitleDrawer.cpp
/**
 * @file TitleDrawer.cpp
 * @brief Implements the drawing logic for the plot title and subtitle.
 */
#include "TitleDrawer.hpp"
#include "../FontManager.hpp"
#include "../Colour.hpp"
#include "../PlotBase.hpp" // For MARGIN_TOP
#include "../utils/Formatters.hpp"
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
    
    // Calculate total width of subtitle for centering
    std::string avg_prefix = _(" (Average: ");
    std::string avg_suffix = "";
    std::string channels_separator = "";
    std::string channels_prefix = "";
    std::string final_suffix = ")";
    
    const auto& channels = opts.raw_channels;
    std::vector<DataSource> avg_channels_to_draw;
    std::vector<DataSource> individual_channels_to_draw;

    if (channels.R) individual_channels_to_draw.push_back(DataSource::R);
    if (channels.G1) individual_channels_to_draw.push_back(DataSource::G1);
    if (channels.G2) individual_channels_to_draw.push_back(DataSource::G2);
    if (channels.B) individual_channels_to_draw.push_back(DataSource::B);

    if (channels.avg_mode == AvgMode::Full) {
        avg_channels_to_draw = {DataSource::R, DataSource::G1, DataSource::G2, DataSource::B};
    } else if (channels.avg_mode == AvgMode::Selected) {
        avg_channels_to_draw = individual_channels_to_draw;
    }

    if (!avg_channels_to_draw.empty() && !individual_channels_to_draw.empty()) {
        channels_separator = " & ";
        channels_prefix = _("Channels: ");
    } else if (avg_channels_to_draw.empty() && !individual_channels_to_draw.empty()) {
        avg_prefix = " ("; // No "Average" part
        channels_prefix = _("Channels: ");
    }
    
    // Get total width for centering
    double total_width = 0;
    cairo_text_extents(cr, avg_prefix.c_str(), &extents); total_width += extents.x_advance;
    for(size_t i = 0; i < avg_channels_to_draw.size(); ++i) {
        cairo_text_extents(cr, Formatters::DataSourceToString(avg_channels_to_draw[i]).c_str(), &extents); total_width += extents.x_advance;
        if (i < avg_channels_to_draw.size() - 1) { cairo_text_extents(cr, ",", &extents); total_width += extents.x_advance; }
    }
    cairo_text_extents(cr, channels_separator.c_str(), &extents); total_width += extents.x_advance;
    cairo_text_extents(cr, channels_prefix.c_str(), &extents); total_width += extents.x_advance;
    for(size_t i = 0; i < individual_channels_to_draw.size(); ++i) {
        cairo_text_extents(cr, Formatters::DataSourceToString(individual_channels_to_draw[i]).c_str(), &extents); total_width += extents.x_advance;
        if (i < individual_channels_to_draw.size() - 1) { cairo_text_extents(cr, ",", &extents); total_width += extents.x_advance; }
    }
    cairo_text_extents(cr, final_suffix.c_str(), &extents); total_width += extents.x_advance;
    
    current_x = ctx.base_width / 2.0 - total_width / 2.0;
    current_y += extents.height + font_manager.calculateScaledSize(5.0);

    // Draw subtitle part by part
    auto draw_text_part = [&](const std::string& text) {
        cairo_move_to(cr, current_x, current_y);
        cairo_show_text(cr, text.c_str());
        cairo_text_extents_t part_extents;
        cairo_text_extents(cr, text.c_str(), &part_extents);
        current_x += part_extents.x_advance;
    };

    auto draw_channels = [&](const std::vector<DataSource>& channels_to_draw) {
        for (size_t i = 0; i < channels_to_draw.size(); ++i) {
            PlotColors::SetSourceFromChannel(cr, channels_to_draw[i]);
            draw_text_part(Formatters::DataSourceToString(channels_to_draw[i]));
            if (i < channels_to_draw.size() - 1) {
                PlotColors::cairo_set_source_grey_50(cr);
                draw_text_part(",");
            }
        }
    };

    if (!avg_channels_to_draw.empty() || !individual_channels_to_draw.empty())
    {
        PlotColors::cairo_set_source_grey_50(cr);
        draw_text_part(avg_prefix);
        draw_channels(avg_channels_to_draw);
        
        PlotColors::cairo_set_source_grey_50(cr);
        draw_text_part(channels_separator);
        draw_text_part(channels_prefix);
        
        draw_channels(individual_channels_to_draw);
        
        PlotColors::cairo_set_source_grey_50(cr);
        draw_text_part(final_suffix);
    }
}

} // namespace DynaRange::Graphics::Drawing