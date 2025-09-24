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
#include <libintl.h>

#define _(string) gettext(string)

// Constants from the R script
constexpr int NCOLS = 11;
constexpr int NROWS = 7;
constexpr int DIMX = 2592;
constexpr int DIMY = 1944;
constexpr int OFFSETX = 100;
constexpr int OFFSETY = 100;
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

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White background
    cairo_paint(cr);

    double patch_width = static_cast<double>(DIMX - 2 * OFFSETX) / (NCOLS);
    double patch_height = static_cast<double>(DIMY - 2 * OFFSETY) / (NROWS);

    int patch_index = 0;
    for (int row = 0; row < NROWS; ++row) {
        for (int col = 0; col < NCOLS; ++col) {
            double t = static_cast<double>(patch_index) / (NCOLS * NROWS - 1);
            double intensity = std::pow(1.0 - t, invgamma);

            double x = OFFSETX + (col * patch_width);
            double y = OFFSETY + (row * patch_height);
            
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

    // The logic to write the file to PNG is now delegated.
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