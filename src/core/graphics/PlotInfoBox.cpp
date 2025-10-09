// File: src/core/graphics/PlotInfoBox.cpp
/**
 * @file PlotInfoBox.cpp
 * @brief Implements the PlotInfoBox class.
 */
#include "PlotInfoBox.hpp"
#include "PlotBase.hpp" // For MARGIN_TOP, MARGIN_LEFT
#include "Colour.hpp"   // For PlotColors
#include "FontManager.hpp" // For FontManager

void PlotInfoBox::AddItem(const std::string& label, const std::string& value, const std::string& annotation) {
    m_items.push_back({label, value, annotation});
}

void PlotInfoBox::Draw(cairo_t* cr, const DynaRange::Graphics::RenderContext& ctx) const {
    if (m_items.empty()) {
        return;
    }
    
    // Set text style using the FontManager
    const DynaRange::Graphics::FontManager font_manager(ctx);
    font_manager.SetInfoBoxFont(cr);

    const double start_x = MARGIN_LEFT + 15.0;
    // Scale line height proportionally
    const double line_height =  font_manager.calculateScaledSize(20.0);

    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        double current_x = start_x;
        double y_pos = MARGIN_TOP + line_height * (i + 1);

        // 1. Draw Label and Value in default color (Gray)
        PlotColors::cairo_set_source_grey_50(cr);
        std::string main_text = item.label + ": " + item.value;
        cairo_move_to(cr, current_x, y_pos);
        cairo_show_text(cr, main_text.c_str());

        // 2. If an annotation exists, draw it in Red
        if (!item.annotation.empty()) {
            cairo_text_extents_t extents;
            cairo_text_extents(cr, main_text.c_str(), &extents);
            current_x += extents.x_advance; // Move cursor to the end of the main text

            // Set color to Red for the annotation
            PlotColors::cairo_set_source_red(cr);
            cairo_move_to(cr, current_x, y_pos);
            cairo_show_text(cr, item.annotation.c_str());
        }
    }
}
