// File: src/core/graphics/ChartGenerator.cpp
/**
 * @file ChartGenerator.cpp
 * @brief Implements the test chart generation logic.
 */
#include "ChartGenerator.hpp"
#include <cairo/cairo.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <libintl.h>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants from the R script, adapted for C++
constexpr int NCOLS = 10;
constexpr int NROWS = 8;
constexpr int DIMX = 2592;
constexpr int DIMY = 1944;
constexpr double ALPHA = 0.8; // Effective area of the chart
constexpr double RADIUS = 20.0; // Radius for the white corner circles
constexpr double RGBMAX = 255.0;

bool GenerateTestChart(
    const std::string& output_filename,
    int R, int G, int B,
    double invgamma,
    std::ostream& log_stream
) {
    if (R < 0 || R > 255 || G < 0 || G > 255 || B < 0 || B > 255 || invgamma <= 0.0) {
        log_stream << _("Error: Invalid chart parameters.") << std::endl;
        return false;
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, DIMX, DIMY);
    cairo_t* cr = cairo_create(surface);

    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        log_stream << _("Error: Could not create Cairo context for chart generation.") << std::endl;
        cairo_surface_destroy(surface);
        return false;
    }

    // Fill the background with black
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    // --- Calculations for effective area, corners, and offsets ---
    const double DIMXc = DIMX * ALPHA;
    const double DIMYc = DIMY * ALPHA;
    const double OFFSETX = (DIMX - DIMXc) / 2.0;
    const double OFFSETY = (DIMY - DIMYc) / 2.0;

    // Position of 4 white circles: top-left, bottom-left, bottom-right, top-right
    std::vector<double> x0 = {OFFSETX, OFFSETX, DIMX - OFFSETX, DIMX - OFFSETX};
    std::vector<double> y0 = {OFFSETY, DIMY - OFFSETY, DIMY - OFFSETY, OFFSETY};

    // --- Draw blue guide lines (0, 0, 191) ---
    cairo_set_source_rgb(cr, 0.0 / 255.0, 0.0 / 255.0, 191.0 / 255.0);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, x0[0], y0[0]);
    cairo_line_to(cr, x0[1], y0[1]);
    cairo_line_to(cr, x0[2], y0[2]);
    cairo_line_to(cr, x0[3], y0[3]);
    cairo_close_path(cr);
    cairo_stroke(cr);
    
    // --- Draw 4 white corner circles ---
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White color
    for (size_t i = 0; i < 4; ++i) {
        cairo_arc(cr, x0[i], y0[i], RADIUS, 0, 2 * M_PI);
        cairo_fill(cr);
    }
    
    // --- Draw color patches inside the effective area ---
    const double patch_width = DIMXc / (NCOLS + 1);
    const double patch_height = DIMYc / (NROWS + 1);

    int patch_index = 0;
    for (int row = 0; row < NROWS; ++row) {
        for (int col = 0; col < NCOLS; ++col) {
            double t = static_cast<double>(patch_index) / (NCOLS * NROWS - 1);
            double intensity = std::pow(1.0 - t, invgamma);

            double x = (col * patch_width) + OFFSETX + (patch_width / 2.0);
            double y = (row * patch_height) + OFFSETY + (patch_height / 2.0);
            
            cairo_set_source_rgb(
                cr,
                (intensity * R) / RGBMAX,
                (intensity * G) / RGBMAX,
                (intensity * B) / RGBMAX
            );

            cairo_rectangle(cr, x, y, patch_width, patch_height);
            cairo_fill(cr);

            patch_index++;
        }
    }

    cairo_status_t status = cairo_surface_write_to_png(surface, output_filename.c_str());
    bool success = (status == CAIRO_STATUS_SUCCESS);

    if (success) {
        log_stream << _("Test chart generated successfully: ") << output_filename << std::endl;
    } else {
        log_stream << _("Error: Failed to write chart to file: ") << output_filename << std::endl;
    }

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return success;
}