/**
 * @file src/core/graphics/Plotting.cpp
 * @brief Implements the high-level plot generation logic for SNR curves.
 *
 * This module provides functions to generate complete PNG plot images using
 * the low-level drawing functions from PlotBase and PlotData. It handles
 * canvas creation, coordinate bounds calculation, and file output.
 */

#include "Plotting.hpp"
#include "../analysis/Analysis.hpp" // Needed for CurveData struct
#include "PlotBase.hpp"
#include "PlotData.hpp"
#include <cairo/cairo.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <optional>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

/**
 * @brief Generates and saves a single SNR plot for one RAW file.
 *
 * This function creates a complete PNG image containing:
 * - A styled plot base with axes, grid, title, and thresholds.
 * - The fitted polynomial curve and data points for the given curve.
 * - A label on the curve (e.g., "ISO 200").
 * - An optional command-line string showing how the plot was generated.
 *
 * Additionally, it adds a timestamp in the bottom-left corner indicating when
 * the plot was generated.
 *
 * @param output_filename The full path where the output PNG file will be saved.
 * @param plot_title The main title of the plot (e.g., "iso00200.dng (OM-1, ISO 200)").
 * @param curve_label The simple label to draw on the curve itself (e.g., "ISO 200").
 * @param signal_ev A vector of signal values converted to EV (logarithmic scale).
 * @param snr_db A vector of corresponding SNR values in dB.
 * @param poly_coeffs The coefficients of the polynomial fit used to draw the curve.
 * @param opts The program options, including plot_mode and generated_command.
 * @param log_stream The output stream for logging messages.
 */
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
        log_stream << "  - Warning: Skipping plot for \"" << plot_title << "\" due to insufficient data points (" << signal_ev.size() << ")." << std::endl;
        return;
    }

    // Create Cairo surface and context
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << "  - Error: Failed to create cairo context for plot \"" << plot_title << "\"." << std::endl;
        cairo_surface_destroy(surface);
        return;
    }

    // Calculate plot bounds based on data
    auto min_max_ev = std::minmax_element(signal_ev.begin(), signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;
    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_data) - 1.0;
    bounds["max_ev"] = (max_ev_data < 0.0) ? 0.0 : ceil(max_ev_data) + 1.0;
    bounds["min_db"] = -15.0;
    bounds["max_db"] = 25.0;

    // Draw the static plot base (axes, grid, labels, thresholds)
    DrawPlotBase(cr, "SNR Curve - " + plot_title, bounds, opts.generated_command, opts.snr_thresholds_db);

    // Draw the dynamic data (curve, points, label)
    std::vector<CurveData> single_curve_vec = {{
        plot_title,         // filename (temporary placeholder)
        curve_label,        // plot_label (the actual label to display)
        "",                 // camera_model (not needed here)
        signal_ev,          // signal_ev
        snr_db,             // snr_db
        poly_coeffs,        // poly_coeffs
        opts.generated_command // generated_command
    }};
    DrawCurvesAndData(cr, single_curve_vec, bounds);

    // --- NEW: Add "Generated at" timestamp ---
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
    timestamp_ss << std::put_time(&local_tm, "Generated at %Y-%m-%d %H:%M:%S");
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

    // Write PNG and clean up
    cairo_surface_write_to_png(surface, output_filename.c_str());
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    log_stream << "  - Info: Plot saved to: " << output_filename << std::endl;
}

/**
 * @brief Generates and saves a summary plot containing all SNR curves.
 *
 * This function creates a comprehensive overview plot comparing all processed
 * ISO files. It uses the same styling and layout as individual plots but overlays
 * multiple curves on the same axes.
 *
 * It also includes the "Generated at" timestamp in the bottom-left corner.
 *
 * @param output_filename The full path where the summary plot PNG will be saved.
 * @param camera_name The name of the camera model (used in the plot title).
 * @param all_curves A vector of CurveData objects, each representing an ISO curve.
 * @param opts The program options, including plot_mode and generated_command.
 * @param log_stream The output stream for logging messages.
 * @return An optional string containing the path to the generated plot, or std::nullopt if skipped.
 */
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
        log_stream << "  - Warning: Skipping summary plot due to no curve data." << std::endl;
        return std::nullopt;
    }

    // Create Cairo surface and context
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << "  - Error: Failed to create cairo context for summary plot." << std::endl;
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

    // Calculate global bounds across all curves
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

    // Draw the static plot base
    std::string title = "SNR Curves - Summary (" + camera_name + ")";
    DrawPlotBase(cr, title, bounds, opts.generated_command, opts.snr_thresholds_db);

    // Draw all curves dynamically
    DrawCurvesAndData(cr, all_curves, bounds);

    // --- NEW: Add "Generated at" timestamp ---
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &time_now);
#else
    localtime_r(&time_now, &local_tm);
#endif
    std::ostringstream timestamp_ss;
    timestamp_ss << std::put_time(&local_tm, "Generated at %Y-%m-%d %H:%M:%S");
    std::string generated_at_text = timestamp_ss.str();

    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, generated_at_text.c_str(), &ext);

    double x = 20;
    double y = PLOT_HEIGHT - 20;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, generated_at_text.c_str());

    // Write PNG and clean up
    cairo_surface_write_to_png(surface, output_filename.c_str());
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    log_stream << "  - Info: Summary Plot saved to: " << output_filename << std::endl;

    return output_filename;
}