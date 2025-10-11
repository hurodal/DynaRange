// File: src/core/graphics/ChartGenerator.hpp
/**
 * @file ChartGenerator.hpp
 * @brief Declares functions to generate a test chart image for dynamic range analysis.
 */
#pragma once
#include "../arguments/ChartOptionsParser.hpp" // For ChartGeneratorOptions
#include <string>
#include <ostream>
#include <vector>
#include <optional>

// Forward declare wxImage to avoid including wxWidgets headers in this core file.
class wxImage;

/**
 * @struct InMemoryImage
 * @brief A generic, library-agnostic container for raw image data.
 */
struct InMemoryImage {
    std::vector<unsigned char> data; ///< Raw pixel data in RGB format.
    int width;                       ///< Image width in pixels.
    int height;                      ///< Image height in pixels.
};

/**
 * @brief Generates and saves a full-size test chart PNG image to a file.
 * @param opts A struct containing all validated chart parameters.
 * @param output_filename The path where the PNG file will be saved.
 * @param log_stream An output stream for logging messages.
 * @return true if the chart was generated successfully, false otherwise.
 */
bool GenerateTestChart(const ChartGeneratorOptions& opts, const std::string& output_filename, std::ostream& log_stream);

/**
 * @brief Generates a small, in-memory thumbnail of a test chart.
 * @param opts A struct containing all validated chart parameters.
 * @param thumb_width The desired width of the thumbnail in pixels.
 * @return An optional containing the generated thumbnail data, or nullopt on failure.
 */
std::optional<InMemoryImage> GenerateChartThumbnail(const ChartGeneratorOptions& opts, int thumb_width);