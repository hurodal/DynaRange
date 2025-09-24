// File: src/core/graphics/ChartGenerator.hpp
/**
 * @file ChartGenerator.hpp
 * @brief Declares the function to generate a test chart image for dynamic range analysis.
 */
#pragma once
#include <string>
#include <ostream>

/**
 * @brief Generates a test chart PNG image.
 * @param output_filename The path where the PNG file will be saved.
 * @param R Red channel value for the brightest patch (0-255).
 * @param G Green channel value for the brightest patch (0-255).
 * @param B Blue channel value for the brightest patch (0-255).
 * @param invgamma The inverse gamma value for the patch intensity curve.
 * @param dim_x Width of the chart in pixels.
 * @param aspect_w Aspect ratio width component.
 * @param aspect_h Aspect ratio height component.
 * @param patches_m Number of rows of patches.
 * @param patches_n Number of columns of patches.
 * @param log_stream An output stream for logging messages.
 * @return true if the chart was generated successfully, false otherwise.
 */
bool GenerateTestChart(
    const std::string& output_filename,
    int R, int G, int B, double invgamma,
    int dim_x, int aspect_w, int aspect_h,
    int patches_m, int patches_n,
    std::ostream& log_stream
);