// File: src/core/graphics/PlotInfoBox.cpp
/**
 * @file PlotInfoBox.cpp
 * @brief Implements the PlotInfoBox class.
 */
#include "PlotInfoBox.hpp"
#include "PlotBase.hpp" // For MARGIN_TOP, MARGIN_LEFT
#include "Colour.hpp"   // For PlotColors

void PlotInfoBox::AddItem(const std::string& label, const std::string& value) {
    m_items.push_back({label, value});
}

void PlotInfoBox::Draw(cairo_t* cr) const {
    if (m_items.empty()) {
        return;
    }
    
    // Set text style: small, gray, and legible
    PlotColors::cairo_set_source_grey_50(cr); // Medium gray color
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 14.0);

    const double x_pos = MARGIN_LEFT + 15.0;
    const double line_height = 20.0;
    
    for (size_t i = 0; i < m_items.size(); ++i) {
        std::string text_to_draw = m_items[i].first + ": " + m_items[i].second;
        double y_pos = MARGIN_TOP + line_height * (i + 1);

        cairo_move_to(cr, x_pos, y_pos);
        cairo_show_text(cr, text_to_draw.c_str());
    }
}