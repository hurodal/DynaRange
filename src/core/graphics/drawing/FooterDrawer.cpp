// File: src/core/graphics/drawing/FooterDrawer.cpp
/**
 * @file FooterDrawer.cpp
 * @brief Implements the drawing logic for plot footer elements.
 */
#include "FooterDrawer.hpp"
#include "../FontManager.hpp"
#include "../Colour.hpp"
#include "../PlotBase.hpp" // For margins
#include <libintl.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#define _(string) gettext(string)

namespace DynaRange::Graphics::Drawing {

void FooterDrawer::Draw(cairo_t* cr, const RenderContext& ctx, const std::string& command_text) const
{
    const FontManager font_manager(ctx);
    
    // --- Command Text ---
    if (!command_text.empty()) {
        PlotColors::cairo_set_source_grey_50(cr);
        font_manager.SetCommandFont(cr);
        cairo_text_extents_t cmd_extents;
        cairo_text_extents(cr, command_text.c_str(), &cmd_extents);
        cairo_move_to(cr, ctx.base_width - MARGIN_RIGHT - cmd_extents.width - 10, ctx.base_height - 15);
        cairo_show_text(cr, command_text.c_str());
    }

    // --- Timestamp ---
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
    #ifdef _WIN32
        localtime_s(&local_tm, &time_now);
    #else
        localtime_r(&time_now, &local_tm);
    #endif
    std::ostringstream timestamp_ss;
    timestamp_ss << std::put_time(&local_tm, _("Generated at %Y-%m-%d %H:%M:%S"));
    std::string generated_at_text = timestamp_ss.str();

    font_manager.SetTimestampFont(cr);
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
    cairo_move_to(cr, 20, ctx.base_height - 15);
    cairo_show_text(cr, generated_at_text.c_str());
}

} // namespace DynaRange::Graphics::Drawing