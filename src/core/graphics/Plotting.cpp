// File: src/core/graphics/Plotting.cpp
/**
 * @file src/core/graphics/Plotting.cpp
 * @brief Implements the high-level plot generation logic for SNR curves.
 *
 * This module provides functions to generate complete PNG plot images using
 * the low-level drawing functions from PlotBase and PlotData. It handles
 * canvas creation, coordinate bounds calculation, and file output.
 */

#include "Plotting.hpp"
#include "PlotBase.hpp"
#include "PlotData.hpp"
#include "PlotInfoBox.hpp"
#include "../io/OutputWriter.hpp"
#include <cairo/cairo.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <optional>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <libintl.h>

#define _(string) gettext(string)

namespace { // Anonymous namespace for private helper functions

/**
 * @brief Draws a "Generated at" timestamp in the bottom-left corner of the plot.
 * @details This function retrieves the current system time, formats it, and
 * draws it using a standard style and position.
 * @param cr The cairo drawing context.
 */
void DrawGeneratedTimestamp(cairo_t* cr) {
    // Get current date/time as formatted string
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

    // Draw timestamp in bottom-left corner
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4); // Same gray as command text

    cairo_text_extents_t ext;
    cairo_text_extents(cr, generated_at_text.c_str(), &ext);

    // Position: bottom-left, 20px from left and 20px from bottom
    double x = 20;
    double y = PLOT_HEIGHT - 20;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, generated_at_text.c_str());
}

/**
 * @brief (New private function) Encapsulates the common plot generation logic.
 * @details This function handles the entire Cairo workflow: surface creation,
 * drawing all plot components (base, data, timestamp), and writing the final
 * PNG file. It is called by the public-facing Generate...Plot functions.
 * @return An optional string containing the path to the generated plot on success.
 */
std::optional<std::string> GeneratePlotInternal(
    const std::string& output_filename,
    const std::string& title,
    const std::vector<CurveData>& curves_to_plot,
    const ProgramOptions& opts,
    const std::map<std::string, double>& bounds,
    std::ostream& log_stream)
{
    // 1. Create Cairo surface and context
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("  - Error: Failed to create cairo context for plot \"") << title << "\"." << std::endl;
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

    // 2. Create and populate the PlotInfoBox
    PlotInfoBox info_box;
    std::stringstream black_ss, sat_ss;
    black_ss << std::fixed << std::setprecision(2) << opts.dark_value;
    sat_ss << std::fixed << std::setprecision(2) << opts.saturation_value;
    info_box.AddItem(_("Black"), black_ss.str());
    info_box.AddItem(_("Saturation"), sat_ss.str());

    // 3. Draw all components
    std::string command_text = curves_to_plot.empty() ? "" : curves_to_plot[0].generated_command;
    DrawPlotBase(cr, title, opts, bounds, command_text, opts.snr_thresholds_db);
    DrawCurvesAndData(cr, info_box, curves_to_plot, bounds);
    DrawGeneratedTimestamp(cr);

    // 4. Write PNG and clean up
    bool success = OutputWriter::WritePng(surface, output_filename, log_stream);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    if (success) {
        return output_filename;
    }
    return std::nullopt;
}

} // end anonymous namespace

// Esta función ya existía y ha sido modificada.
void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& plot_title,
    const std::string& curve_label,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    // Do not generate plot if plot_mode is 0
    if (opts.plot_mode == 0) {
        return;
    }

    if (signal_ev.size() < 2) {
        log_stream << _("  - Warning: Skipping plot for \"") << plot_title << _("\" due to insufficient data points (") << signal_ev.size() << ")." << std::endl;
        return;
    }

    // 1. Calculate plot bounds specific to this single curve
    auto min_max_ev = std::minmax_element(signal_ev.begin(), signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;
    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_data) - 1.0;
    bounds["max_ev"] = (max_ev_data < 0.0) ? 0.0 : ceil(max_ev_data) + 1.0;
    bounds["min_db"] = -15.0;
    bounds["max_db"] = 25.0;

    // 2. Prepare the data for a single curve
    std::vector<CurveData> single_curve_vec = {{
        plot_title,
        curve_label,
        "", // camera_model not needed for individual plot title
        signal_ev,
        snr_db,
        poly_coeffs,
        opts.generated_command
    }};

    // 3. Delegate the entire drawing process to the internal helper function
    GeneratePlotInternal(output_filename, _("SNR Curve - ") + plot_title, single_curve_vec, opts, bounds, log_stream);
}

// Esta función ya existía y ha sido modificada.
std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_filename,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    // Do not generate plot if plot_mode is 0
    if (opts.plot_mode == 0) {
        return std::nullopt;
    }

    if (all_curves.empty()) {
        log_stream << _("  - Warning: Skipping summary plot due to no curve data.") << std::endl;
        return std::nullopt;
    }

    // 1. Calculate global bounds across all curves
    double min_ev_global = 1e6, max_ev_global = -1e6;
    for (const auto& curve : all_curves) {
        if (!curve.signal_ev.empty()) {
            min_ev_global = std::min(min_ev_global, *std::min_element(curve.signal_ev.begin(), curve.signal_ev.end()));
            max_ev_global = std::max(max_ev_global, *std::max_element(curve.signal_ev.begin(), curve.signal_ev.end()));
        }
    }

    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_global) - 1.0;
    bounds["max_ev"] = (max_ev_global < 0.0) ? 0.0 : ceil(max_ev_global) + 1.0;
    bounds["min_db"] = -15.0;
    bounds["max_db"] = 25.0;

    // 2. Prepare title
    std::string title = _("SNR Curves - Summary (") + camera_name + ")";

    // 3. Delegate the entire drawing process to the internal helper function
    return GeneratePlotInternal(output_filename, title, all_curves, opts, bounds, log_stream);
}