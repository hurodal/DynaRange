/**
 * @file core/graphics/Plotting.cpp
 * @brief Implements the high-level plot generation logic.
 */
#include "Plotting.hpp"
#include "../Analysis.hpp" // Needed for CurveData struct
#include "Drawing.hpp"
#include <cairo/cairo.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <optional>

namespace fs = std::filesystem;

void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& image_title,
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
        log_stream << "  - Warning: Skipping plot for \"" << image_title << "\" due to insufficient data points (" << signal_ev.size() << ")." << std::endl;
        return;
    }
    
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << "  - Error: Failed to create cairo context for plot \"" << image_title << "\"." << std::endl;
        cairo_surface_destroy(surface);
        return;
    }

    auto min_max_ev = std::minmax_element(signal_ev.begin(), signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;
    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_data) - 1.0;
    bounds["max_ev"] = (max_ev_data < 0.0) ? 0.0 : ceil(max_ev_data) + 1.0;
    bounds["min_db"] = -15.0;
    bounds["max_db"] = 25.0;

    // Pass the SNR thresholds to the drawing function
    DrawPlotBase(cr, "SNR Curve - " + image_title, bounds, opts.generated_command, opts.snr_thresholds_db);
    std::vector<CurveData> single_curve_vec = {{image_title, "", signal_ev, snr_db, poly_coeffs, opts.generated_command}};
    DrawCurvesAndData(cr, single_curve_vec, bounds);
    
    cairo_surface_write_to_png(surface, output_filename.c_str());
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    log_stream << "  - Info: Plot saved to: " << output_filename << std::endl;
}

std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_dir,
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

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PLOT_WIDTH, PLOT_HEIGHT);
    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << "  - Error: Failed to create cairo context for summary plot." << std::endl;
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

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

    std::string title = "SNR Curves - Summary (" + camera_name + ")";
    std::string safe_camera_name = camera_name;
    std::replace(safe_camera_name.begin(), safe_camera_name.end(), ' ', '_');
    std::string output_filename = (fs::path(output_dir) / ("DR_summary_plot_" + safe_camera_name + ".png")).string();

    // Pass the SNR thresholds to the drawing function
    DrawPlotBase(cr, title, bounds, opts.generated_command, opts.snr_thresholds_db);
    DrawCurvesAndData(cr, all_curves, bounds);

    cairo_surface_write_to_png(surface, output_filename.c_str());
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    log_stream << "  - Info: Summary Plot saved to: " << output_filename << std::endl;
    
    return output_filename;
}