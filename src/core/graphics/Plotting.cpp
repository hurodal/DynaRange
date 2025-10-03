// File: src/core/graphics/Plotting.cpp
/**
 * @file src/core/graphics/Plotting.cpp
 * @brief Implements the high-level plot generation logic for SNR curves.
 *
 * This module provides functions to generate complete plot images using
 * the low-level drawing functions from PlotBase and PlotData. It handles
 * canvas creation, coordinate bounds calculation, and file output.
 */

#include "Plotting.hpp"
#include "PlotBase.hpp"
#include "PlotData.hpp"
#include "PlotInfoBox.hpp"
#include "../io/OutputWriter.hpp"
#include "PlotDataGenerator.hpp" 
#include "../Constants.hpp"
#include "../utils/Formatters.hpp"
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h> // Added missing header for PDF support
#include <cairo/cairo-svg.h>  // Added missing header for SVG support
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

namespace fs = std::filesystem;

namespace { // Anonymous namespace for private helper functions

void DrawGeneratedTimestamp(cairo_t* cr) {
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

    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, generated_at_text.c_str(), &ext);

    double x = 20;
    double y = PlotDefs::BASE_HEIGHT - 20;

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, generated_at_text.c_str());
}

std::optional<std::string> GeneratePlotInternal(
    const std::string& output_filename,
    const std::string& title,
    const std::vector<CurveData>& curves_to_plot,
    const std::vector<DynamicRangeResult>& results_to_plot,
    const ProgramOptions& opts,
    const std::map<std::string, double>& bounds,
    std::ostream& log_stream)
{
    cairo_surface_t *surface = nullptr;

    switch (DynaRange::Constants::PLOT_FORMAT) {
        case DynaRange::Constants::PlotOutputFormat::PDF:
            surface = cairo_pdf_surface_create(output_filename.c_str(), PlotDefs::WIDTH, PlotDefs::HEIGHT);
            break;
        case DynaRange::Constants::PlotOutputFormat::SVG:
            surface = cairo_svg_surface_create(output_filename.c_str(), PlotDefs::WIDTH, PlotDefs::HEIGHT);
            break;
        case DynaRange::Constants::PlotOutputFormat::PNG:
        default:
            surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PlotDefs::WIDTH, PlotDefs::HEIGHT);
            break;
    }

    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("  - Error: Failed to create cairo context for plot \"") << title << "\"." << std::endl;
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

    if (PlotDefs::SCALE != 1.0) {
        cairo_scale(cr, PlotDefs::SCALE, PlotDefs::SCALE);
    }

    PlotInfoBox info_box;
    std::stringstream black_ss, sat_ss;
    
    black_ss << std::fixed << std::setprecision(2) << opts.dark_value;
    std::string black_annotation = opts.black_level_is_default ? _(" (estimated)") : "";
    info_box.AddItem(_("Black"), black_ss.str(), black_annotation);

    sat_ss << std::fixed << std::setprecision(2) << opts.saturation_value;
    std::string sat_annotation = opts.saturation_level_is_default ? _(" (estimated)") : "";
    info_box.AddItem(_("Saturation"), sat_ss.str(), sat_annotation);

    std::string command_text = curves_to_plot.empty() ? "" : curves_to_plot[0].generated_command;
    DrawPlotBase(cr, title, opts, bounds, command_text, opts.snr_thresholds_db);
    DrawCurvesAndData(cr, info_box, curves_to_plot, results_to_plot, bounds);
    DrawGeneratedTimestamp(cr);
    
    bool success = false;
    switch (DynaRange::Constants::PLOT_FORMAT) {
        case DynaRange::Constants::PlotOutputFormat::PDF:
            cairo_show_page(cr); // Finalize the page for PDF
            // Fall through to common vector finalization
        case DynaRange::Constants::PlotOutputFormat::SVG:
            cairo_surface_flush(surface);
            success = (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS); // Check status on surface
            if (success) {
                log_stream << _("  - Info: Plot saved to: ") << output_filename << std::endl;
            }
            break;
        case DynaRange::Constants::PlotOutputFormat::PNG:
        default:
            success = OutputWriter::WritePng(surface, output_filename, log_stream);
            break;
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    
    if (success) {
        return output_filename;
    }
    return std::nullopt;
}

} // end anonymous namespace

void GenerateSnrPlot(
    const std::string& output_filename,
    const std::string& plot_title,
    const std::string& curve_label,
    DataSource channel,
    const std::vector<double>& signal_ev,
    const std::vector<double>& snr_db,
    const cv::Mat& poly_coeffs,
    const DynamicRangeResult& dr_result,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    if (opts.plot_mode == 0) {
        return;
    }

    if (signal_ev.size() < 2) {
        log_stream << _("  - Warning: Skipping plot for \"") << plot_title << _("\" due to insufficient data points (") << signal_ev.size() << ")."
 << std::endl;
        return;
    }

    auto min_max_ev = std::minmax_element(signal_ev.begin(), signal_ev.end());
    double min_ev_data = *min_max_ev.first;
    double max_ev_data = *min_max_ev.second;

    auto min_max_db = std::minmax_element(snr_db.begin(), snr_db.end());
    double min_db_data = *min_max_db.first;
    double max_db_data = *min_max_db.second;

    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_data) - 1.0;
    bounds["max_ev"] = (max_ev_data < 0.0) ? 0.0 : ceil(max_ev_data) + 1.0;
    bounds["min_db"] = floor(min_db_data / 5.0) * 5.0;
    bounds["max_db"] = ceil(max_db_data / 5.0) * 5.0;

    CurveData single_curve_data;
    single_curve_data.filename = plot_title;
    single_curve_data.channel = channel;
    single_curve_data.plot_label = curve_label;
    single_curve_data.camera_model = "";
    
    std::vector<PointData> points;
    points.reserve(signal_ev.size());
    for(size_t i = 0; i < signal_ev.size(); ++i) {
        points.push_back({signal_ev[i], snr_db[i], channel});
    }
    single_curve_data.points = points;
    
    single_curve_data.poly_coeffs = poly_coeffs;
    single_curve_data.generated_command = opts.generated_command;
    
    single_curve_data.curve_points = PlotDataGenerator::GenerateCurvePoints(single_curve_data);

    std::vector<CurveData> single_curve_vec = {single_curve_data};
    std::vector<DynamicRangeResult> single_result_vec = {dr_result};

    GeneratePlotInternal(output_filename, _("SNR Curve - ") + plot_title, single_curve_vec, single_result_vec, opts, bounds, log_stream);
}

std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_filename,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    if (opts.plot_mode == 0) {
        log_stream << "\n" << _("Plot generation skipped as per user request (--plot 0).") << std::endl;
        return std::nullopt;
    }

    if (all_curves.empty()) {
        log_stream << _("  - Warning: Skipping summary plot due to no curve data.") << std::endl;
        return std::nullopt;
    }

    std::vector<CurveData> curves_with_points = all_curves;
    for (auto& curve : curves_with_points) {
        curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
    }

    double min_ev_global = 1e6, max_ev_global = -1e6;
    double min_db_global = 1e6, max_db_global = -1e6;
    for (const auto& curve : curves_with_points) {
        if (!curve.points.empty()) {
            auto minmax_ev_it = std::minmax_element(curve.points.begin(), curve.points.end(),
                [](const PointData& a, const PointData& b){ return a.ev < b.ev; });
            min_ev_global = std::min(min_ev_global, minmax_ev_it.first->ev);
            max_ev_global = std::max(max_ev_global, minmax_ev_it.second->ev);

            auto minmax_db_it = std::minmax_element(curve.points.begin(), curve.points.end(),
                [](const PointData& a, const PointData& b){ return a.snr_db < b.snr_db; });
            min_db_global = std::min(min_db_global, minmax_db_it.first->snr_db);
            max_db_global = std::max(max_db_global, minmax_db_it.second->snr_db);
        }
    }

    std::map<std::string, double> bounds;
    bounds["min_ev"] = floor(min_ev_global) - 1.0;
    bounds["max_ev"] = (max_ev_global < 0.0) ? 0.0 : ceil(max_ev_global) + 1.0;
    bounds["min_db"] = floor(min_db_global / 5.0) * 5.0;
    bounds["max_db"] = ceil(max_db_global / 5.0) * 5.0;
    
    std::string title = _("SNR Curves - Summary (") + camera_name + ")";
    
    return GeneratePlotInternal(output_filename, title, curves_with_points, all_results, opts, bounds, log_stream);
}

std::map<std::string, std::string> GenerateIndividualPlots(
    const std::vector<CurveData>& all_curves_data,
    const std::vector<DynamicRangeResult>& all_dr_results,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream)
{
    std::map<std::string, std::string> plot_paths_map;
    if (opts.plot_mode == 0) return plot_paths_map;

    log_stream << "\n" << _("Generating individual SNR plots...") << std::endl;

    std::map<std::string, std::vector<CurveData>> curves_by_file;
    std::map<std::string, std::vector<DynamicRangeResult>> results_by_file;
    for (const auto& curve : all_curves_data) {
        curves_by_file[curve.filename].push_back(curve);
    }
    for (const auto& result : all_dr_results) {
        results_by_file[result.filename].push_back(result);
    }

    for (auto& pair : curves_by_file) {
        const std::string& filename = pair.first;
        std::vector<CurveData>& curves_for_this_file = pair.second;
        const std::vector<DynamicRangeResult>& results_for_this_file = results_by_file[filename];

        if (curves_for_this_file.empty()) continue;

        for (auto& curve : curves_for_this_file) {
            curve.curve_points = PlotDataGenerator::GenerateCurvePoints(curve);
        }

        const auto& first_curve = curves_for_this_file[0];
        fs::path plot_path = paths.GetIndividualPlotPath(first_curve);
        plot_paths_map[filename] = plot_path.string();

        std::stringstream title_ss;
        title_ss << fs::path(filename).filename().string();
        if (!first_curve.camera_model.empty()) {
            title_ss << " (" << first_curve.camera_model;
            if (first_curve.iso_speed > 0) {
                title_ss << ", " << _("ISO ") << static_cast<int>(first_curve.iso_speed);
            }
            title_ss << ")";
        }

        double min_ev = 1e6, max_ev = -1e6, min_db = 1e6, max_db = -1e6;
        for (const auto& curve : curves_for_this_file) {
            if (!curve.points.empty()) {
                auto minmax_ev_it = std::minmax_element(curve.points.begin(), curve.points.end(),
                    [](const PointData& a, const PointData& b){ return a.ev < b.ev; });
                min_ev = std::min(min_ev, minmax_ev_it.first->ev);
                max_ev = std::max(max_ev, minmax_ev_it.second->ev);

                auto minmax_db_it = std::minmax_element(curve.points.begin(), curve.points.end(),
                    [](const PointData& a, const PointData& b){ return a.snr_db < b.snr_db; });
                min_db = std::min(min_db, minmax_db_it.first->snr_db);
                max_db = std::max(max_db, minmax_db_it.second->snr_db);
            }
        }
        
        std::map<std::string, double> bounds;
        bounds["min_ev"] = floor(min_ev) - 1.0;
        bounds["max_ev"] = (max_ev < 0.0) ? 0.0 : ceil(max_ev) + 1.0;
        bounds["min_db"] = floor(min_db / 5.0) * 5.0;
        bounds["max_db"] = ceil(max_db / 5.0) * 5.0;

        GeneratePlotInternal(plot_path.string(), title_ss.str(), curves_for_this_file, results_for_this_file, opts, bounds, log_stream);
    }
    return plot_paths_map;
}