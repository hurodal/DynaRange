// File: src/core/arguments/ArgumentsOptions.hpp
/**
 * @file src/core/arguments/ArgumentsOptions.hpp
 * @brief Defines the data structure and types for program configuration.
 */
#pragma once
#include "../graphics/Constants.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstddef> // For std::size

// Default values for program options remain here as they are directly tied to arguments.
constexpr double DEFAULT_BLACK_LEVEL = 256.0;
constexpr double DEFAULT_SATURATION_LEVEL = 4095.0;
constexpr double DEFAULT_PATCH_RATIO = 0.5;
constexpr double DEFAULT_SNR_THRESHOLD_DB = 12.0;
constexpr double DEFAULT_DR_NORMALIZATION_MPX = 0.0;
constexpr int DEFAULT_PLOT_MODE = 0;
constexpr int DEFAULT_POLY_ORDER = 3;
constexpr const char* DEFAULT_OUTPUT_FILENAME = "results.csv";

// Available polynomial orders for curve fitting.
constexpr int VALID_POLY_ORDERS[] = {2, 3};
/**
 * @brief Helper function to convert a UI selection index to a polynomial order value.
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
 * @struct PlottingDetails
 * @brief Holds boolean flags for which components of a plot to draw.
 */
struct PlottingDetails {
    bool show_scatters = true; ///< Draw the individual (EV, SNR_dB) data points.
    bool show_curve = true;    ///< Draw the fitted polynomial curve.
    bool show_labels = true;   ///< Draw the DR value labels at the threshold intersections.
};

/**
 * @struct RawChannelSelection
 * @brief Holds the boolean selection for which RAW channels to analyze.
 */
struct RawChannelSelection {
    bool R = false;
    bool G1 = false;
    bool G2 = false;
bool B = false;
    bool AVG = true; // Default behavior is to average all channels.
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
// The old 'plot_mode' is replaced by more descriptive members
    bool generate_plot = false;
DynaRange::Graphics::Constants::PlotOutputFormat plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PNG;
    int plot_command_mode = 0; // 0: No plot, 1: No command, 2: Short, 3: Long
    PlottingDetails plot_details;

    bool create_chart_mode = false;
std::vector<std::string> chart_colour_params;
    std::vector<int> chart_params;
    std::vector<double> chart_coords;
    std::vector<int> chart_patches;
    // [M, N]
    RawChannelSelection raw_channels;
// New member for channel selection
    std::string generated_command;
    std::map<std::string, std::string> plot_labels;
    double sensor_resolution_mpx = 0.0;
    std::string print_patch_filename;
// --- Internal Flags ---
    bool black_level_is_default = true;
    bool saturation_level_is_default = true;
int GetChartPatchesM() const { return chart_patches.size() >= 1 ? chart_patches[0] : 4;
}
    int GetChartPatchesN() const { return chart_patches.size() >= 2 ? chart_patches[1] : 6; }
};