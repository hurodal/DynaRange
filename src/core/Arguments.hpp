/**
 * @file Arguments.hpp
 * @brief Defines the structures and functions for command-line argument management.
 * @author (Your Name)
 * @date 2025-09-10
 */
#pragma once

#include <string>
#include <vector>
#include <ostream>
#include <optional>

/// @brief Default polynomial order for curve fitting.
constexpr int DEFAULT_POLY_ORDER = 3;

/**
 * @enum CommandFormat
 * @brief Specifies the desired format for the generated command string.
 */
enum class CommandFormat {
    Full, ///< Complete command with all paths and arguments, for the GUI.
    
    /**
     * @brief Abbreviated command for plots.
     * @note Uses long argument names (--param) for clarity, shortens paths,
     * and omits irrelevant arguments like output and input files.
     */
    Plot  
};

/**
 * @struct ProgramOptions
 * @brief Holds all the configuration options for the dynamic range analysis.
 */
struct ProgramOptions {
    double dark_value; ///< Manual black level value.
    double saturation_value; ///< Manual saturation level value.
    std::string dark_file_path; ///< Path to the dark frame RAW file.
    std::string sat_file_path; ///< Path to the saturation frame RAW file.
    std::string output_filename; ///< Path for the output CSV results file.
    std::vector<std::string> input_files; ///< List of input RAW files for analysis.
    int poly_order = DEFAULT_POLY_ORDER; ///< Polynomial order for the SNR curve fit.
    
    double dr_normalization_mpx; ///< Megapixel count for DR normalization.
    std::vector<double> snr_thresholds_db; ///< SNR thresholds in dB to calculate DR for.

    double patch_ratio; ///< Relative area of the chart patches to use for analysis.
    int plot_mode; ///< Plot generation mode (0=no, 1=plot, 2=plot+command).

    bool create_chart_mode = false; ///< Flag to activate chart creation mode.
    std::vector<double> chart_params; ///< Parameters for chart creation (R, G, B, gamma).

    std::string generated_command; ///< Stores the generated command string for plots.
};

/**
 * @brief Parses command-line arguments using CLI11.
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @return A ProgramOptions struct populated with the parsed values.
 */
ProgramOptions ParseArguments(int argc, char* argv[]);

/**
 * @brief Generates an equivalent command-line string from a ProgramOptions struct.
 * @param opts The populated ProgramOptions struct.
 * @param format The desired output format (Full for GUI, Plot for graphics).
 * @return The generated command-line string.
 */
std::string GenerateCommandString(const ProgramOptions& opts, CommandFormat format = CommandFormat::Full);