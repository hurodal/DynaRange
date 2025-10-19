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
#include <cstddef>

// Default values (complete definitions are needed for context, even if unchanged)
constexpr double DEFAULT_BLACK_LEVEL = 256.0;
constexpr double DEFAULT_SATURATION_LEVEL = 4095.0;
constexpr double DEFAULT_PATCH_RATIO = 0.5;
constexpr double DEFAULT_SNR_THRESHOLD_DB = 12.0;
constexpr double DEFAULT_DR_NORMALIZATION_MPX = 0.0;
constexpr int DEFAULT_PLOT_MODE = 0; // Note: This might be obsolete if plotingChoice controls mode
constexpr int DEFAULT_POLY_ORDER = 3;
constexpr const char* DEFAULT_OUTPUT_FILENAME = "results.csv";
constexpr const char* DEFAULT_PRINT_PATCHES_FILENAME = "printpatches.png";
constexpr const char* DEFAULT_CHART_FILENAME = "magentachart.png";
const std::vector<double> DEFAULT_SNR_THRESHOLDS_DB = { 12.0, 0.0 };
constexpr int VALID_POLY_ORDERS[] = {2, 3};
constexpr int DEFAULT_CHART_PATCHES_M = 4; 
constexpr int DEFAULT_CHART_PATCHES_N = 6;

/**
 * @brief Helper function to get polynomial order from index.
 * @param index The index selected in the choice control (0 or 1).
 * @return The corresponding polynomial order (2 or 3), or the default.
 */
inline int PolyOrderFromIndex(int index) {
    if (index >= 0 && static_cast<size_t>(index) < std::size(VALID_POLY_ORDERS)) {
        return VALID_POLY_ORDERS[index];
    }
    return DEFAULT_POLY_ORDER; // Fallback to a safe default
}


// Enums and Structs (complete definitions needed for context)
/**
 * @enum CommandFormat
 * @brief Specifies the desired format for the generated command string.
 */
enum class CommandFormat {
    Full,       ///< Full command with absolute paths.
    PlotShort,  ///< Short command for plot footer (short args, filenames only).
    PlotLong,   ///< Long command for plot footer (long args, filenames only).
    GuiPreview  ///< Command preview for GUI (long args, full paths).
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
 * @enum AvgMode
 * @brief Specifies the averaging mode for RAW channels.
 */
enum class AvgMode {
    None = 0,     ///< Do not calculate an average.
    Full = 1,     ///< Average all four RAW channels (R, G1, G2, B).
    Selected = 2  ///< Average only the channels explicitly selected by the user.
};

/**
 * @struct RawChannelSelection
 * @brief Holds the boolean selection for which RAW channels to analyze.
 */
struct RawChannelSelection {
    bool R = false;  ///< Analyze Red channel.
    bool G1 = false; ///< Analyze Green1 channel.
    bool G2 = false; ///< Analyze Green2 channel.
    bool B = false;  ///< Analyze Blue channel.
    /** @brief Averaging mode (None, Full, Selected). Defaults to Full. */
    AvgMode avg_mode = AvgMode::Full;
};

/**
 * @struct ProgramOptions
 * @brief Holds all the configuration options for the dynamic range analysis.
 * @details This structure is populated from command-line arguments or GUI settings.
 */
struct ProgramOptions {
    // --- Core Analysis Settings ---
    /** @brief Black level value used for normalization. */
    double dark_value = DEFAULT_BLACK_LEVEL;
    /** @brief Saturation level value used for normalization. */
    double saturation_value = DEFAULT_SATURATION_LEVEL;
    /** @brief Path to the dark frame RAW file (optional). */
    std::string dark_file_path;
    /** @brief Path to the saturation frame RAW file (optional). */
    std::string sat_file_path;
    /** @brief List of input RAW file paths for analysis. */
    std::vector<std::string> input_files;
    /** @brief Order of the polynomial fit for SNR curves (2 or 3). */
    int poly_order = DEFAULT_POLY_ORDER;
    /** @brief Target resolution in megapixels for DR normalization (0.0 for per-pixel). */
    double dr_normalization_mpx = DEFAULT_DR_NORMALIZATION_MPX;
    /** @brief List of SNR thresholds (in dB) for DR calculation. */
    std::vector<double> snr_thresholds_db = DEFAULT_SNR_THRESHOLDS_DB; // Initialize with default
    /** @brief Relative area (0.0-1.0) of the center of each patch to sample. */
    double patch_ratio = DEFAULT_PATCH_RATIO;
    /** @brief Selection state for analyzing individual and averaged RAW channels. */
    RawChannelSelection raw_channels;
    /** @brief Sensor resolution in megapixels (detected or assumed). */
    double sensor_resolution_mpx = 0.0;
    /** @brief Detected width of the RAW image active area. */
    int raw_width = 0;
    /** @brief Detected height of the RAW image active area. */
    int raw_height = 0;
    /** @brief Detected full width of the RAW sensor data (including masked areas). */
    int full_raw_width = 0;
    /** @brief Detected full height of the RAW sensor data (including masked areas). */
    int full_raw_height = 0;
    /** @brief Index of the file used for corner/patch detection. */
    int source_image_index = 0;

    // --- Output Settings ---
    /** @brief Base filename (or full path) for the output CSV file. */
    std::string output_filename = DEFAULT_OUTPUT_FILENAME;
    /** @brief If true, generate SNR curve plots. */
    bool generate_plot = false;
    /** @brief Output format for generated plots (PNG, PDF, SVG). */
    DynaRange::Graphics::Constants::PlotOutputFormat plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PNG;
    /** @brief Controls display of command line in plot footer (0=No plot, 1=No command, 2=Short, 3=Long). */
    int plot_command_mode = 3; // Default to long command if plotting
    /** @brief Controls which elements (scatters, curve, labels) are drawn on the plot. */
    PlottingDetails plot_details = { true, true, true }; // Initialize with defaults
    /** @brief If true, generate individual plot files for each input RAW file. */
    bool generate_individual_plots = false;
    /** @brief Filename for the debug patch overlay image (empty if not requested, may contain sentinel value). */
    std::string print_patch_filename = "_USE_DEFAULT_PRINT_PATCHES_"; // Initialize with sentinel
    /** @brief Map of input filenames to labels used in plots. */
    std::map<std::string, std::string> plot_labels;
    /** @brief Stores the generated equivalent command string. */
    std::string generated_command;

    // --- Chart Generation/Reading Settings ---
    /** @brief If true, run in chart generation mode instead of analysis mode. */
    bool create_chart_mode = false;
    /** @brief Parameters for chart color generation (--chart-colour). */
    std::vector<std::string> chart_colour_params;
    /** @brief Parameters for chart dimension/format generation (--chart). */
    std::vector<int> chart_params;
    /** @brief Manually specified chart corner coordinates (--chart-coords). */
    std::vector<double> chart_coords;
    /** @brief Manually specified chart patch grid dimensions (--chart-patches). */
    std::vector<int> chart_patches = {DEFAULT_CHART_PATCHES_M, DEFAULT_CHART_PATCHES_N}; // Initialize with defaults

    // --- Internal Flags (set during processing/parsing) ---
    /** @brief True if the black level was estimated or defaulted, false if user-provided. */
    bool black_level_is_default = true;
    /** @brief True if the saturation level was estimated or defaulted, false if user-provided. */
    bool saturation_level_is_default = true;

    // --- NEW MEMBERS FOR GUI CONFIGURATION ---
    /** @brief Name for camera manually entered in the GUI. */
    std::string gui_manual_camera_name = "";
    /** @brief Flag reflecting GUI checkbox state for using EXIF vs manual camera name. */
    bool gui_use_exif_camera_name = true; // Default GUI state matches common expectation
    /** @brief Flag reflecting GUI checkbox state for adding camera name suffix to filenames. */
    bool gui_use_camera_suffix = true; // Default GUI state

    // --- Helper Methods ---
    /** @brief Gets the number of patch rows (M) from chart_patches or default. */
    int GetChartPatchesM() const { return chart_patches.size() >= 1 ? chart_patches[0] : DEFAULT_CHART_PATCHES_M; }
    /** @brief Gets the number of patch columns (N) from chart_patches or default. */
    int GetChartPatchesN() const { return chart_patches.size() >= 2 ? chart_patches[1] : DEFAULT_CHART_PATCHES_N; }
};