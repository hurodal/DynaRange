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

/// @brief Default polynomial order for curve fitting.
constexpr int DEFAULT_POLY_ORDER = 3;

// MODIFIED: Added a constant for the default output filename.
// This is now the single source of truth for this value.
constexpr const char* DEFAULT_OUTPUT_FILENAME = "results.csv";

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
    Full,       ///< Complete command with full paths and all arguments.
    PlotShort,  ///< Abbreviated command for plots with short argument names (-f, -r).
    PlotLong,   ///< Abbreviated command for plots with long argument names (--poly-fit, --patch-ratio).
    GuiPreview  ///< Command for the GUI preview: long names and full paths for copy-pasting.
};

/**
 * @struct ProgramOptions
 * @brief Holds all the configuration options for the dynamic range analysis.
 */
struct ProgramOptions {
    double dark_value = 0.0;
    double saturation_value = 16383.0;
    std::string dark_file_path;
    std::string sat_file_path;
    std::string output_filename = DEFAULT_OUTPUT_FILENAME;
    std::vector<std::string> input_files;
    int poly_order = DEFAULT_POLY_ORDER;
    double dr_normalization_mpx = 8.0;
    std::vector<double> snr_thresholds_db;
    double patch_ratio = 0.5;
    int plot_mode = 0;
    bool create_chart_mode = false;
    std::vector<std::string> chart_colour_params;
    std::vector<int> chart_params;
    std::vector<double> chart_coords;
    std::vector<int> chart_patches; // [M, N]
    std::string generated_command;
    std::map<std::string, std::string> plot_labels;
    double sensor_resolution_mpx = 0.0;

    // Getters para mantener la compatibilidad con la GUI y ChartProfile
    int GetChartPatchesM() const { return chart_patches.size() >= 1 ? chart_patches[0] : 4; }
    int GetChartPatchesN() const { return chart_patches.size() >= 2 ? chart_patches[1] : 6; }
};