// File: src/core/graphics/FontManager.cpp
/**
 * @file FontManager.cpp
 * @brief Implements the FontManager class.
 */
#include "FontManager.hpp"
#include "Constants.hpp" // For PlotDefs::FILE_BASE_WIDTH

namespace DynaRange::Graphics {

// --- Base font sizes designed for the reference width (FILE_BASE_WIDTH) ---
namespace BaseFontSizes {
    constexpr double TITLE = 24.0;
    constexpr double SUBTITLE = 18.0;
    constexpr double AXIS_LABEL = 20.0;
    constexpr double AXIS_TICK = 16.0;
    constexpr double THRESHOLD_LABEL = 16.0;
    constexpr double CURVE_LABEL = 14.0;
    constexpr double DR_VALUE = 12.0;
    constexpr double COMMAND = 12.0;
    constexpr double TIMESTAMP = 12.0;
    constexpr double INFO_BOX = 13.0;
}
FontManager::FontManager(const RenderContext& ctx) : m_ctx(ctx) {}

double FontManager::calculateScaledSize(double base_size) const {
    // Scale font size proportionally to the canvas width, using the file width as the reference.
    return (base_size * m_ctx.base_width) / static_cast<double>(Constants::PlotDefs::BASE_WIDTH);
}

void FontManager::SetTitleFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::TITLE));
}

void FontManager::SetSubtitleFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::SUBTITLE));
}

void FontManager::SetAxisLabelFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::AXIS_LABEL));
}

void FontManager::SetAxisTickFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::AXIS_TICK));
}

void FontManager::SetThresholdLabelFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::THRESHOLD_LABEL));
}

void FontManager::SetCurveLabelFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::CURVE_LABEL));
}

void FontManager::SetDrValueFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::DR_VALUE));
}

void FontManager::SetCommandFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::COMMAND));
}

void FontManager::SetTimestampFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::TIMESTAMP));
}

void FontManager::SetInfoBoxFont(cairo_t* cr) const {
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, calculateScaledSize(BaseFontSizes::INFO_BOX));
}

} // namespace DynaRange::Graphics