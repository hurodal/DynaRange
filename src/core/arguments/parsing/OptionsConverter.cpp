// File: src/core/arguments/parsing/OptionsConverter.cpp
/**
 * @file OptionsConverter.cpp
 * @brief Implements the converter from parsed arguments to ProgramOptions.
 */
#include "OptionsConverter.hpp"
#include "../Constants.hpp"
#include "../ArgumentsOptions.hpp"       // For ProgramOptions, enums, defaults
#include "../../graphics/Constants.hpp" // For PlotOutputFormat enum
#include <stdexcept>   
#include <algorithm>   
#include <string>      
#include <vector>      
#include <map>         
#include <any>         

namespace { // Anonymous namespace for internal helper

/**
 * @brief Helper to safely get a typed value from the std::any map.
 * @tparam T The desired type.
 * @param key The key (long argument name) for the value.
 * @param values The map of parsed argument values.
 * @return The value cast to type T.
 * @throw std::runtime_error if the key is not found or the type cast fails.
 */
template <typename T>
T Get(const std::string& key, const std::map<std::string, std::any>& values)
{
    // Check if the key exists in the map
    auto it = values.find(key);
    if (it != values.end()) {
        try {
            // Attempt to cast the std::any value to the requested type T
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast& e) {
            // Throw an error if the type cast fails
            throw std::runtime_error("Invalid type requested for argument '" + key + "': " + e.what());
        }
    }
    // Throw an error if the argument key was not found in the map
    throw std::runtime_error("Argument not found in map: " + key);
}

} // end anonymous namespace

namespace DynaRange::Arguments::Parsing {

/**
 * @brief Converts the map of parsed argument values into a ProgramOptions struct.
 * @param values The map where keys are long argument names and values are std::any holding parsed data.
 * @return A populated ProgramOptions struct ready for use by the application core.
 */
ProgramOptions OptionsConverter::ToProgramOptions(const std::map<std::string, std::any>& values)
{
    using namespace DynaRange::Arguments::Constants;
    ProgramOptions opts; // Initialize with defaults from ArgumentsOptions.hpp

    // --- Populate options from the 'values' map using the Get helper ---
    opts.create_chart_mode = Get<bool>(CreateChartMode, values);
    opts.chart_params = Get<std::vector<int>>(Chart, values);
    opts.chart_colour_params = Get<std::vector<std::string>>(ChartColour, values);
    opts.chart_coords = Get<std::vector<double>>(ChartCoords, values);
    opts.chart_patches = Get<std::vector<int>>(ChartPatches, values);
    opts.dark_value = Get<double>(BlackLevel, values);
    opts.saturation_value = Get<double>(SaturationLevel, values);
    opts.dark_file_path = Get<std::string>(BlackFile, values);
    opts.sat_file_path = Get<std::string>(SaturationFile, values);
    opts.output_filename = Get<std::string>(OutputFile, values);
    opts.input_files = Get<std::vector<std::string>>(InputFiles, values);
    opts.poly_order = Get<int>(PolyFit, values);
    opts.dr_normalization_mpx = Get<double>(DrNormalizationMpx, values);
    opts.patch_ratio = Get<double>(PatchRatio, values);

    // Plotting options
    opts.generate_plot = Get<bool>(GeneratePlot, values);
    if (opts.generate_plot) {
        // Determine Plot Format
        std::string format_str = Get<std::string>(PlotFormat, values);
        std::transform(format_str.begin(), format_str.end(), format_str.begin(), ::toupper); // Case-insensitive
        if (format_str == "SVG") {
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::SVG;
        } else if (format_str == "PDF") {
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PDF;
        } else { // Default to PNG if not recognized or is "PNG"
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PNG;
        }

        // Parse Plot Parameters (S C L Cmd)
        auto params_vec = Get<std::vector<int>>(PlotParams, values);
        if (params_vec.size() == 4) {
            opts.plot_details.show_scatters = (params_vec[0] != 0);
            opts.plot_details.show_curve = (params_vec[1] != 0);
            opts.plot_details.show_labels = (params_vec[2] != 0);
            // Clamp command mode between 1 and 3 if outside range, else use value
            int cmd_mode = params_vec[3];
            opts.plot_command_mode = (cmd_mode < 1 || cmd_mode > 3) ? 3 : cmd_mode; // Default to 3 (long) if invalid
        } else { // Fallback to defaults if vector size is wrong
            // Reset plot details to default values
            opts.plot_details = { true, true, true };
            opts.plot_command_mode = 3; // Default to long command
        }
    } else {
        // Ensure command mode is consistent if plotting is disabled
        opts.plot_command_mode = 0; // Set to "No plot"
        // Reset plot details to default values as well? Current behavior keeps last parsed/default.
        // opts.plot_details = { true, true, true }; // Optional: Reset details too
    }

    // Print Patches filename (sentinel or user-provided)
    opts.print_patch_filename = Get<std::string>(PrintPatches, values);

    // Internal flags
    opts.black_level_is_default = Get<bool>(BlackLevelIsDefault, values);
    opts.saturation_level_is_default = Get<bool>(SaturationLevelIsDefault, values);

    // SNR Thresholds (handle default case)
    if (Get<bool>(SnrThresholdIsDefault, values)) {
        opts.snr_thresholds_db = DEFAULT_SNR_THRESHOLDS_DB;
    } else {
        opts.snr_thresholds_db = Get<std::vector<double>>(SnrThresholdDb, values);
        // Add validation? Ensure thresholds are reasonable?
    }

    // Raw Channels and Average Mode
    auto channels_vec = Get<std::vector<int>>(RawChannels, values);
    // Expect 5 values: R G1 G2 B AVG_MODE
    if (channels_vec.size() == 5) {
        opts.raw_channels.R = (channels_vec[0] != 0);
        opts.raw_channels.G1 = (channels_vec[1] != 0);
        opts.raw_channels.G2 = (channels_vec[2] != 0);
        opts.raw_channels.B = (channels_vec[3] != 0);

        int avg_val = channels_vec[4];
        // Validate the average mode value against the AvgMode enum range
        if (avg_val >= static_cast<int>(AvgMode::None) && avg_val <= static_cast<int>(AvgMode::Selected)) {
            opts.raw_channels.avg_mode = static_cast<AvgMode>(avg_val);
        } else {
            // Invalid value provided, fallback to the default average mode
            opts.raw_channels.avg_mode = AvgMode::Full; // Revert to default
        }
    } else {
        // Handle error: vector size incorrect. Fallback to default channel selection.
        opts.raw_channels = {}; // Re-initialize to default RawChannelSelection state (AvgMode::Full)
    }


    // --- Populate NEW GUI-specific members ---
    // Read the values from the map using the internal names defined in Constants.hpp
    // These values originate from GuiPresenter::UpdateManagerFromView
    opts.gui_manual_camera_name = Get<std::string>(GuiManualCameraName, values);
    opts.gui_use_exif_camera_name = Get<bool>(GuiUseExifNameFlag, values);
    opts.gui_use_camera_suffix = Get<bool>(GuiUseSuffixFlag, values);

    // Note: Fields like generated_command, plot_labels, sensor_resolution_mpx, raw dimensions
    // are populated later during the initialization phase (InitializeAnalysis), not directly from arguments.

    return opts;
}

} // namespace DynaRange::Arguments::Parsing