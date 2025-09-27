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
#include <vector>

#define _(string) gettext(string)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Anonymous namespace for internal helpers
namespace {

// This is a new helper function created to avoid code duplication (DRY).
// It contains the core Cairo drawing logic shared by both public functions.
cairo_surface_t *CreateChartSurface(const ChartGeneratorOptions &opts) {
    
  if (opts.R < 0 || opts.R > 255 || opts.G < 0 || opts.G > 255 || opts.B < 0 ||
      opts.B > 255 || opts.invgamma <= 0.0 || opts.dim_x <= 0 ||
      opts.aspect_w <= 0 || opts.aspect_h <= 0 || opts.patches_m <= 0 ||
      opts.patches_n <= 0) {
    return nullptr;
  }

  const int DIMX = opts.dim_x;
  const int DIMY = static_cast<int>(
      round(static_cast<double>(DIMX) * opts.aspect_h / opts.aspect_w));
  const int NROWS = opts.patches_m;
  const int NCOLS = opts.patches_n;

  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, DIMX, DIMY);
  cairo_t *cr = cairo_create(surface);

  if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(surface);
    cairo_destroy(cr);
    return nullptr;
  }

  // Constants for drawing
  constexpr double ALPHA = 0.8;
  constexpr double RGBMAX = 255.0;

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Black background
  cairo_paint(cr);

  const double DIMXc = DIMX * ALPHA;
  const double DIMYc = DIMY * ALPHA;
  const double OFFSETX = (DIMX - DIMXc) / 2.0;
  const double OFFSETY = (DIMY - DIMYc) / 2.0;
  std::vector<double> x0 = {OFFSETX, OFFSETX, DIMX - OFFSETX, DIMX - OFFSETX};
  std::vector<double> y0 = {OFFSETY, DIMY - OFFSETY, DIMY - OFFSETY, OFFSETY};

  // The radius calculation is now aligned with the R script: 1% of the image
  // diagonal.
  const double diag = std::sqrt(static_cast<double>(DIMX * DIMX + DIMY * DIMY));
  const double RADIUS = diag * 0.01;

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
      double intensity = std::pow(1.0 - t, opts.invgamma);
      double x = (col * patch_width) + OFFSETX + (patch_width / 2.0);
      double y = (row * patch_height) + OFFSETY + (patch_height / 2.0);
      cairo_set_source_rgb(cr, (intensity * opts.R) / RGBMAX,
                           (intensity * opts.G) / RGBMAX,
                           (intensity * opts.B) / RGBMAX);
      cairo_rectangle(cr, x, y, patch_width, patch_height);
      cairo_fill(cr);
      patch_index++;
    }
  }

  cairo_destroy(cr);
  return surface;
}

} // end anonymous namespace

// Its logic is now simplified to call the helper function and write the file.
bool GenerateTestChart(const ChartGeneratorOptions &opts,
                       const std::string &output_filename,
                       std::ostream &log_stream) {
  cairo_surface_t *surface = CreateChartSurface(opts);
  if (!surface) {
    log_stream << _("Error: Invalid chart parameters or Cairo context creation "
                    "failed.")
               << std::endl;
    return false;
  }

  bool success = OutputWriter::WritePng(surface, output_filename, log_stream);
  if (success) {
    log_stream << _("Test chart generated successfully: ") << output_filename
               << std::endl;
  } else {
    log_stream << _("Error: Failed to write chart to file: ") << output_filename
               << std::endl;
  }

  cairo_surface_destroy(surface);
  return success;
}

// Its logic is now implemented to generate the preview in memory.
std::optional<InMemoryImage>
GenerateChartThumbnail(const ChartGeneratorOptions &opts, int thumb_width) {
  ChartGeneratorOptions thumb_opts = opts;
  thumb_opts.dim_x = thumb_width;

  cairo_surface_t *surface = CreateChartSurface(thumb_opts);
  if (!surface) {
    return std::nullopt; // Return empty optional on failure
  }

  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  unsigned char *cairo_data = cairo_image_surface_get_data(surface);

  // Create a buffer for our generic RGB image data.
  std::vector<unsigned char> rgb_data(width * height * 3);

  // Convert Cairo's ARGB pixel data to a simple RGB buffer.
  for (int i = 0; i < width * height; ++i) {
    // Cairo BGRA format on little-endian systems
    rgb_data[i * 3 + 0] = cairo_data[i * 4 + 2]; // R
    rgb_data[i * 3 + 1] = cairo_data[i * 4 + 1]; // G
    rgb_data[i * 3 + 2] = cairo_data[i * 4 + 0]; // B
  }

  // Clean up Cairo surface
  cairo_surface_destroy(surface);
  return InMemoryImage{std::move(rgb_data), width, height};
}