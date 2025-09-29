// File: src/core/arguments/ProgramOptions.hpp
/**
 * @file src/core/arguments/ProgramOptions.hpp
 * @brief Defines the data structure and types for program configuration.
 */
#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstddef> // For std::size

// Default values for program options
constexpr double DEFAULT_BLACK_LEVEL = 256.0;
constexpr double DEFAULT_SATURATION_LEVEL = 4095.0;
constexpr double DEFAULT_PATCH_RATIO = 0.5;
constexpr double DEFAULT_SNR_THRESHOLD_DB = 12.0;
constexpr double DEFAULT_DR_NORMALIZATION_MPX = 8.0;
constexpr int DEFAULT_PLOT_MODE = 0;
constexpr int DEFAULT_POLY_ORDER = 3;
constexpr const char* DEFAULT_OUTPUT_FILENAME = "results.csv";
constexpr const char* CLI_EXECUTABLE_NAME = "rango";
constexpr int DEFAULT_MIN_BRIGHTNESS = 40;

// Available polynomial orders for curve fitting.
constexpr int VALID_POLY_ORDERS[] = {2, 3};
/**
 * @brief Helper function to convert a UI selection index to a polynomial order value.
 * @param index The zero-based index from the wxChoice control.
 * @return The corresponding polynomial order (e.g., 2 or 3).
 */
inline int PolyOrderFromIndex(int index) {
    if (index >= 0 && static_cast<size_t>(index) < std::size(VALID_POLY_ORDERS)) {
        return VALID_POLY_ORDERS[index];
    }
    return DEFAULT_POLY_ORDER; // Fallback to a safe default
}

/**
 * @enum CommandFormat
 * @brief Specifies the desired format for the generated command string.
 */
enum class CommandFormat {
    Full,
    PlotShort,
    PlotLong,
    GuiPreview
};
/**
 * @struct ProgramOptions
 * @brief Holds all the configuration options for the dynamic range analysis.
 */
struct ProgramOptions {
    double dark_value = DEFAULT_BLACK_LEVEL;
    double saturation_value = DEFAULT_SATURATION_LEVEL;
    std::string dark_file_path;
    std::string sat_file_path;
    std::string output_filename = DEFAULT_OUTPUT_FILENAME;
    std::vector<std::string> input_files;
    int poly_order = DEFAULT_POLY_ORDER;
    double dr_normalization_mpx = DEFAULT_DR_NORMALIZATION_MPX;
    std::vector<double> snr_thresholds_db;
    double patch_ratio = DEFAULT_PATCH_RATIO;
    int plot_mode = DEFAULT_PLOT_MODE;
    bool create_chart_mode = false;
    std::vector<std::string> chart_colour_params;
    std::vector<int> chart_params;
    std::vector<double> chart_coords;
    std::vector<int> chart_patches;
    // [M, N]
    std::string generated_command;
    std::map<std::string, std::string> plot_labels;
    double sensor_resolution_mpx = 0.0;
    std::string print_patch_filename;
    int min_corner_brightness = DEFAULT_MIN_BRIGHTNESS;
    int GetChartPatchesM() const { return chart_patches.size() >= 1 ? chart_patches[0] : 4;
    }
    int GetChartPatchesN() const { return chart_patches.size() >= 2 ? chart_patches[1] : 6; }
};