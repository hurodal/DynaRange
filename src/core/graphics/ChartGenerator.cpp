// File: src/core/graphics/ChartGenerator.cpp
/**
 * @file ChartGenerator.cpp
 * @brief Implements the test chart generation logic.
 */
#include "ChartGenerator.hpp"
#include "../io/OutputWriter.hpp"
#include <cairo/cairo.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <libintl.h>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Default constants, still used for some elements
constexpr double ALPHA = 0.8; // Effective area of the chart
constexpr double RADIUS = 20.0; // Radius for the white corner circles
constexpr double RGBMAX = 255.0;

bool GenerateTestChart(
    const std::string& output_filename,
    int R, int G, int B, double invgamma,
    int dim_x, int aspect_w, int aspect_h,
    int patches_m, int patches_n, // M=rows, N=cols
    std::ostream& log_stream
) {
    if (R < 0 || R > 255 || G < 0 || G > 255 || B < 0 || B > 255 || invgamma <= 0.0 || dim_x <= 0 || aspect_w <= 0 || aspect_h <= 0 || patches_m <= 0 || patches_n <= 0) {
        log_stream << _("Error: Invalid chart parameters.") << std::endl;
        return false;
    }

    // Use parameters passed to the function, not hardcoded constants.
    const int DIMX = dim_x;
    const int DIMY = static_cast<int>(round(static_cast<double>(DIMX) * aspect_h / aspect_w));
    const int NROWS = patches_m;
    const int NCOLS = patches_n;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, DIMX, DIMY);
    cairo_t* cr = cairo_create(surface);

    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("Error: Could not create Cairo context for chart generation.") << std::endl;
        cairo_surface_destroy(surface);
        return false;
    }

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black background
    cairo_paint(cr);

    const double DIMXc = DIMX * ALPHA;
    const double DIMYc = DIMY * ALPHA;
    const double OFFSETX = (DIMX - DIMXc) / 2.0;
    const double OFFSETY = (DIMY - DIMYc) / 2.0;

    std::vector<double> x0 = {OFFSETX, OFFSETX, DIMX - OFFSETX, DIMX - OFFSETX};
    std::vector<double> y0 = {OFFSETY, DIMY - OFFSETY, DIMY - OFFSETY, OFFSETY};

    cairo_set_source_rgb(cr, 0.0, 0.0, 191.0 / 255.0);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, x0[0], y0[0]);
    cairo_line_to(cr, x0[1], y0[1]);
    cairo_line_to(cr, x0[2], y0[2]);
    cairo_line_to(cr, x0[3], y0[3]);
    cairo_close_path(cr);
    cairo_stroke(cr);
    
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    for (size_t i = 0; i < 4; ++i) {
        cairo_arc(cr, x0[i], y0[i], RADIUS, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    const double patch_width = DIMXc / (NCOLS + 1);
    const double patch_height = DIMYc / (NROWS + 1);

    int patch_index = 0;
    for (int row = 0; row < NROWS; ++row) {
        for (int col = 0; col < NCOLS; ++col) {
            double t = static_cast<double>(patch_index) / (NCOLS * NROWS - 1);
            double intensity = std::pow(1.0 - t, invgamma);
            double x = (col * patch_width) + OFFSETX + (patch_width / 2.0);
            double y = (row * patch_height) + OFFSETY + (patch_height / 2.0);
            
            cairo_set_source_rgb(cr, (intensity * R) / RGBMAX, (intensity * G) / RGBMAX, (intensity * B) / RGBMAX);
            cairo_rectangle(cr, x, y, patch_width, patch_height);
            cairo_fill(cr);
            patch_index++;
        }
    }

    bool success = OutputWriter::WritePng(surface, output_filename, log_stream);
    if (success) {
        log_stream << _("Test chart generated successfully: ") << output_filename << std::endl;
    } else {
        log_stream << _("Error: Failed to write chart to file: ") << output_filename << std::endl;
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return success;
}