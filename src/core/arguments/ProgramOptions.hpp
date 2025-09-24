// File: src/core/arguments/ProgramOptions.hpp
/**
 * @file src/core/arguments/ProgramOptions.hpp
 * @brief Defines the data structure and types for program configuration.
 */
#pragma once
#include <string>
#include <vector>
#include <map>

/// @brief Default polynomial order for curve fitting.
constexpr int DEFAULT_POLY_ORDER = 3;

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
    double dark_value = 0.0;                       ///< Manual or calculated black level value.
    double saturation_value = 16383.0;             ///< Manual or calculated saturation level value.
    std::string dark_file_path;                    ///< Path to the dark frame RAW file.
    std::string sat_file_path;                     ///< Path to the saturation frame RAW file.
    std::string output_filename;                   ///< Path for the output CSV results file.
    std::vector<std::string> input_files;          ///< List of input RAW files for analysis.
    int poly_order = DEFAULT_POLY_ORDER;           ///< Polynomial order for the SNR curve fit.
    double dr_normalization_mpx = 8.0;             ///< Megapixel count for DR normalization.
    std::vector<double> snr_thresholds_db;         ///< SNR thresholds in dB to calculate DR for.
    double patch_ratio = 0.5;                      ///< Relative area of chart patches to use for analysis.
    int plot_mode = 0;                             ///< Plot generation mode (0=no, 1=plot, 2=short_cmd, 3=long_cmd).
    bool create_chart_mode = false;                ///< Flag to activate chart creation mode.
    std::vector<std::string> chart_colour_params;  ///< Optional parameters for chart colour <R G B invgamma>.
    std::vector<int> chart_params;                 ///< Optional parameters for chart size <DIMX W H>.
    std::vector<int> chart_patches_params;         ///< Optional parameters for chart patches <M N>.
    std::string generated_command;                 ///< Stores the generated command string for plots.
    std::map<std::string, std::string> plot_labels;///< Maps a filename to its desired plot label (e.g., "ISO 100").
    double sensor_resolution_mpx = 0.0;            ///< If 0, try to auto-detect from RAW metadata.
};