// File: src/core/graphics/ChartGenerator.hpp
/**
 * @file ChartGenerator.hpp
 * @brief Declares the function to generate a test chart image for dynamic range analysis.
 * @details This module is responsible for creating a PNG image with a grid of patches,
 * as required by the dynaRange analysis workflow. It is a direct C++ translation
 * of the logic found in the reference R script.
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
 * @param log_stream An output stream for logging messages.
 * @return true if the chart was generated successfully, false otherwise.
 */
bool GenerateTestChart(
    const std::string& output_filename,
    int R, int G, int B,
    double invgamma,
    std::ostream& log_stream
);