// File: src/core/graphics/PlotBase.cpp
/**
 * @file src/core/graphics/PlotBase.cpp
 * @brief Implements the low-level Cairo drawing functions for the plot base.
 */
#include "PlotBase.hpp"
#include "Colour.hpp" 
#include <cmath>
#include <iomanip>
#include <sstream>
#include <libintl.h>
#include <vector>
#include <string>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Anonymous namespace for helper functions in this file
namespace { 

void DrawDashedLine(cairo_t* cr, double x1, double y1, double x2, double y2, double dash_length = 20.0) {
    double dashes[] = {dash_length, dash_length};
    cairo_save(cr);
    cairo_set_dash(cr, dashes, 2, 0);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void DrawPlotBackgroundAndBorder(cairo_t* cr) {
    const int plot_area_width = PlotDefs::BASE_WIDTH - MARGIN_LEFT - MARGIN_RIGHT;
    const int plot_area_height = PlotDefs::BASE_HEIGHT - MARGIN_TOP - MARGIN_BOTTOM;
    
    PlotColors::cairo_set_source_white(cr);
    cairo_rectangle(cr, 0, 0, PlotDefs::BASE_WIDTH, PlotDefs::BASE_HEIGHT);
    cairo_fill(cr);
    
    PlotColors::cairo_set_source_black(cr);
    cairo_set_line_width(cr, 3.0);
    cairo_rectangle(cr, MARGIN_LEFT, MARGIN_TOP, plot_area_width, plot_area_height);
    cairo_stroke(cr);
}

void DrawGridLines(cairo_t* cr, const std::map<std::string, double>& bounds) {
    // NOTE: MapToPixelCoords uses the base dimensions internally.
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };
    
    PlotColors::cairo_set_source_grey_20(cr);
    cairo_set_line_width(cr, 1.0);

    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) {
        auto [p1x, p1y] = map_coords(ev, bounds.at("min_db"));
        auto [p2x, p2y] = map_coords(ev, bounds.at("max_db"));
        cairo_move_to(cr, p1x, p1y); cairo_line_to(cr, p2x, p2y); cairo_stroke(cr);
    }
    
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), db);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), db);
        cairo_move_to(cr, p1x, p1y); cairo_line_to(cr, p2x, p2y); cairo_stroke(cr);
    }
}

void DrawThresholdLines(cairo_t* cr, const std::map<std::string, double>& bounds, const std::vector<double>& snr_thresholds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    cairo_set_line_width(cr, 2.0);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 16.0);

    for(const double threshold : snr_thresholds) {
        auto [p1x, p1y] = map_coords(bounds.at("min_ev"), threshold);
        auto [p2x, p2y] = map_coords(bounds.at("max_ev"), threshold);
        DrawDashedLine(cr, p1x, p1y, p2x, p2y);
        std::stringstream ss;
        ss << "SNR > " << std::fixed << std::setprecision(1) << threshold << "dB";
        cairo_move_to(cr, p1x + 20, p1y - 10);
        cairo_show_text(cr, ss.str().c_str());
    }
}

void DrawXAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    cairo_set_font_size(cr, 16.0);
    cairo_text_extents_t extents;
    for (double ev = ceil(bounds.at("min_ev")); ev <= floor(bounds.at("max_ev")); ev += 1.0) { 
        std::string ev_str = std::to_string((int)ev);
        cairo_text_extents(cr, ev_str.c_str(), &extents);
        auto [px, py] = map_coords(ev, bounds.at("min_db"));
        cairo_move_to(cr, px - extents.width / 2, PlotDefs::BASE_HEIGHT - MARGIN_BOTTOM + 25);
        cairo_show_text(cr, ev_str.c_str());
    }
}

void DrawYAxisLabels(cairo_t* cr, const std::map<std::string, double>& bounds) {
    auto map_coords = [&](double ev, double db) {
        return MapToPixelCoords(ev, db, bounds);
    };

    cairo_set_font_size(cr, 16.0);
    cairo_text_extents_t extents;
    for (double db = ceil(bounds.at("min_db")); db <= floor(bounds.at("max_db")); db += 5.0) { 
        std::string db_str = std::to_string((int)db);
        cairo_text_extents(cr, db_str.c_str(), &extents);
        auto [px, py] = map_coords(bounds.at("min_ev"), db);
        cairo_move_to(cr, MARGIN_LEFT - extents.width - 15, py + extents.height / 2);
        cairo_show_text(cr, db_str.c_str());
    }
}

void DrawPlotAnnotations(cairo_t* cr, const std::string& title, const ProgramOptions& opts, const std::string& command_text) {
    cairo_text_extents_t extents;
    // --- Main Title ---
    PlotColors::cairo_set_source_black(cr);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24.0);
    cairo_text_extents(cr, title.c_str(), &extents);
    double current_x = PlotDefs::BASE_WIDTH / 2.0 - extents.width / 2.0;
    double current_y = MARGIN_TOP - 40;
    cairo_move_to(cr, current_x, current_y);
    cairo_show_text(cr, title.c_str());

    // --- Channel Subtitle ---
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 18.0);
    current_x += extents.x_advance + 10;

    const auto& channels = opts.raw_channels;
    bool has_avg = channels.AVG;
    bool has_r = channels.R, has_g1 = channels.G1, has_g2 = channels.G2, has_b = channels.B;
    bool has_channels = has_r || has_g1 || has_g2 || has_b;

    // Default case: only AVG is selected, which is the most common usage.
    if (has_avg && !has_channels) {
        PlotColors::cairo_set_source_grey_50(cr);
        std::string avg_text = _("(Average channels)");
        cairo_move_to(cr, current_x, current_y);
        cairo_show_text(cr, avg_text.c_str());
    } else if (has_channels) { // Case for "channels only" or "channels + avg"
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

        auto draw_channel_name = [&](const std::string& name, DataSource channel) {
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

    // --- Axis Labels ---
    PlotColors::cairo_set_source_black(cr);
    cairo_set_font_size(cr, 20.0);
    std::string x_label = _("RAW exposure (EV)");
    cairo_text_extents(cr, x_label.c_str(), &extents);
    cairo_move_to(cr, PlotDefs::BASE_WIDTH / 2.0 - extents.width / 2.0, PlotDefs::BASE_HEIGHT - MARGIN_BOTTOM + 70);
    cairo_show_text(cr, x_label.c_str());

    std::string y_label = _("SNR (dB)");
    cairo_text_extents(cr, y_label.c_str(), &extents);
    cairo_save(cr);
    cairo_move_to(cr, MARGIN_LEFT / 2.0 - extents.height / 2.0, PlotDefs::BASE_HEIGHT / 2.0 + extents.width / 2.0);
    cairo_rotate(cr, -M_PI / 2.0);
    cairo_show_text(cr, y_label.c_str());
    cairo_restore(cr);

    // --- Command Text ---
    if (!command_text.empty()) {
        PlotColors::cairo_set_source_grey_50(cr);
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        cairo_text_extents_t cmd_extents;
        cairo_text_extents(cr, command_text.c_str(), &cmd_extents);
        cairo_move_to(cr, PlotDefs::BASE_WIDTH - MARGIN_RIGHT - cmd_extents.width - 10, PlotDefs::BASE_HEIGHT - 20);
        cairo_show_text(cr, command_text.c_str());
    }
}

} // end of anonymous namespace

void DrawPlotBase(
    cairo_t* cr,
    const std::string& title,
    const ProgramOptions& opts,
    const std::map<std::string, double>& bounds,
    const std::string& command_text,
    const std::vector<double>& snr_thresholds)
{
    DrawPlotBackgroundAndBorder(cr);
    DrawGridLines(cr, bounds);
    DrawThresholdLines(cr, bounds, snr_thresholds);
    DrawXAxisLabels(cr, bounds);
    DrawYAxisLabels(cr, bounds);
    DrawPlotAnnotations(cr, title, opts, command_text);
}