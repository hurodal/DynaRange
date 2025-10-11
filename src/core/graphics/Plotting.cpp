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
#include "PlotOrchestrator.hpp"
#include "Constants.hpp"
#include "../io/OutputWriter.hpp"
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace { // Anonymous namespace for private helper functions

std::optional<std::string> GeneratePlotInternal(
    const std::string& output_filename,
    const std::string& title,
    const std::vector<CurveData>& curves_to_plot,
    const std::vector<DynamicRangeResult>& results_to_plot,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    // 1. Create the context for high-resolution file output.
    const auto render_ctx = DynaRange::Graphics::RenderContext{
        DynaRange::Graphics::Constants::PlotDefs::BASE_WIDTH,
        DynaRange::Graphics::Constants::PlotDefs::BASE_HEIGHT
    };

    // 2. Prepare the Cairo surface based on the desired file format.
    bool is_vector = (opts.plot_format == DynaRange::Graphics::Constants::PlotOutputFormat::SVG || opts.plot_format == DynaRange::Graphics::Constants::PlotOutputFormat::PDF);
    double scale = is_vector ? DynaRange::Graphics::Constants::VECTOR_PLOT_SCALE_FACTOR : 1.0;
    int width = static_cast<int>(render_ctx.base_width * scale);
    int height = static_cast<int>(render_ctx.base_height * scale);

    cairo_surface_t *surface = nullptr;
    switch (opts.plot_format) {
        case DynaRange::Graphics::Constants::PlotOutputFormat::SVG:
            surface = cairo_svg_surface_create(output_filename.c_str(), width, height);
            break;
        case DynaRange::Graphics::Constants::PlotOutputFormat::PDF:
            surface = cairo_pdf_surface_create(output_filename.c_str(), width, height);
            break;
        case DynaRange::Graphics::Constants::PlotOutputFormat::PNG:
        default:
            surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
            break;
    }

    cairo_t *cr = cairo_create(surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("  - Error: Failed to create cairo context for plot \"") << title << "\"."
                   << std::endl;
        cairo_surface_destroy(surface);
        return std::nullopt;
    }

    if (scale != 1.0) {
        cairo_scale(cr, scale, scale);
    }
    
    // 3. Call the central "skeleton" function to do all the drawing.
    DynaRange::Graphics::DrawPlotToCairoContext(cr, render_ctx, curves_to_plot, results_to_plot, title, opts);
    
    // 4. Finalize and save the file.
    bool success = false;
    switch (opts.plot_format) {
        case DynaRange::Graphics::Constants::PlotOutputFormat::SVG:
        case DynaRange::Graphics::Constants::PlotOutputFormat::PDF:
            cairo_surface_flush(surface);
            success = (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
            if (success) {
                log_stream << _("  - Info: Plot saved to: ") << output_filename << std::endl;
            }
            break;
        case DynaRange::Graphics::Constants::PlotOutputFormat::PNG:
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
    if (!opts.generate_plot) {
        return;
    }

    if (signal_ev.size() < 2) {
        log_stream << _("  - Warning: Skipping plot for \"") << plot_title << _("\" due to insufficient data points (") << signal_ev.size() << ")."
                   << std::endl;
        return;
    }

    // The logic for calculating 'bounds' has been removed from this function,
    // as it is now handled centrally by the plot orchestrator called by GeneratePlotInternal.

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
    
    // curve_points are now generated inside the central orchestrator,
    // so this line is no longer needed here.
    // single_curve_data.curve_points = PlotDataGenerator::GenerateCurvePoints(single_curve_data);

    std::vector<CurveData> single_curve_vec = {single_curve_data};
    std::vector<DynamicRangeResult> single_result_vec = {dr_result};

    // Corrected call to GeneratePlotInternal without the 'bounds' argument.
    GeneratePlotInternal(output_filename, _("SNR Curve - ") + plot_title, single_curve_vec, single_result_vec, opts, log_stream);
}

std::optional<std::string> GenerateSummaryPlot(
    const std::string& output_filename,
    const std::string& camera_name,
    const std::vector<CurveData>& all_curves,
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    std::ostream& log_stream)
{
    if (!opts.generate_plot) {
        log_stream << "\n" << _("Plot generation skipped as per user request.") << std::endl;
        return std::nullopt;
    }

    if (all_curves.empty()) {
        log_stream << _("  - Warning: Skipping summary plot due to no curve data.") << std::endl;
        return std::nullopt;
    }

    std::string title = _("SNR Curves - Summary (") + camera_name + ")";
    
    return GeneratePlotInternal(output_filename, title, all_curves, all_results, opts, log_stream);
}

std::map<std::string, std::string> GenerateIndividualPlots(
    const std::vector<CurveData>& all_curves_data,
    const std::vector<DynamicRangeResult>& all_dr_results,
    const ProgramOptions& opts,
    const PathManager& paths,
    std::ostream& log_stream)
{
    std::map<std::string, std::string> plot_paths_map;
    if (!opts.generate_plot) return plot_paths_map;

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

        const auto& first_curve = curves_for_this_file[0];
        fs::path plot_path = paths.GetIndividualPlotPath(first_curve, opts);
        
        std::stringstream title_ss;
        title_ss << fs::path(filename).filename().string();
        if (!first_curve.camera_model.empty()) {
            title_ss << " (" << first_curve.camera_model;
            if (first_curve.iso_speed > 0) {
                title_ss << ", " << _("ISO ") << static_cast<int>(first_curve.iso_speed);
            }
            title_ss << ")";
        }
        
        if (auto path_opt = GeneratePlotInternal(plot_path.string(), title_ss.str(), curves_for_this_file, results_for_this_file, opts, log_stream)) {
            plot_paths_map[filename] = *path_opt;
        }
    }
    return plot_paths_map;
}

