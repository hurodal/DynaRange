// File: src/core/arguments/parsing/OptionsConverter.cpp
/**
 * @file OptionsConverter.cpp
 * @brief Implements the converter from parsed arguments to ProgramOptions.
 */
#include "OptionsConverter.hpp"
#include "../Constants.hpp"
#include "../../graphics/Constants.hpp"
#include <stdexcept>
#include <algorithm>

namespace { // Anonymous namespace for internal helper

/**
 * @brief Helper to safely get a typed value from the std::any map.
 * @tparam T The desired type.
 * @param key The key for the value.
 * @param values The map of values.
 * @return The value cast to type T.
 * @throw std::runtime_error if the key is not found or the type is wrong.
 */
template <typename T>
T Get(const std::string& key, const std::map<std::string, std::any>& values)
{
    if (values.count(key)) {
        try {
            return std::any_cast<T>(values.at(key));
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid type requested for argument: " + key);
        }
    }
    throw std::runtime_error("Argument not found in map: " + key);
}

} // end anonymous namespace

namespace DynaRange::Arguments::Parsing {

ProgramOptions OptionsConverter::ToProgramOptions(const std::map<std::string, std::any>& values)
{
    using namespace DynaRange::Arguments::Constants;
    ProgramOptions opts;

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

    opts.generate_plot = Get<bool>(GeneratePlot, values);
    if (opts.generate_plot) {
        std::string format_str = Get<std::string>(PlotFormat, values);
        std::transform(format_str.begin(), format_str.end(), format_str.begin(), ::toupper);
        if (format_str == "SVG")
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::SVG;
        else if (format_str == "PDF")
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PDF;
        else
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PNG;

        auto params_vec = Get<std::vector<int>>(PlotParams, values);
        if (params_vec.size() == 4) {
            opts.plot_details.show_scatters = (params_vec[0] != 0);
            opts.plot_details.show_curve = (params_vec[1] != 0);
            opts.plot_details.show_labels = (params_vec[2] != 0);
            opts.plot_command_mode = params_vec[3];
        } else { // Fallback to default if vector is wrong size
            opts.plot_details = { true, true, true };
            opts.plot_command_mode = 3;
        }
    }

    opts.print_patch_filename = Get<std::string>(PrintPatches, values);
    opts.black_level_is_default = Get<bool>(BlackLevelIsDefault, values);
    opts.saturation_level_is_default = Get<bool>(SaturationLevelIsDefault, values);
    
    if (Get<bool>(SnrThresholdIsDefault, values)) {
        opts.snr_thresholds_db = { 12.0, 0.0 };
    } else {
        opts.snr_thresholds_db = Get<std::vector<double>>(SnrThresholdDb, values);
    }

    auto channels_vec = Get<std::vector<int>>(RawChannels, values);
    if (channels_vec.size() == 5) {
        opts.raw_channels.R = (channels_vec[0] != 0);
        opts.raw_channels.G1 = (channels_vec[1] != 0);
        opts.raw_channels.G2 = (channels_vec[2] != 0);
        opts.raw_channels.B = (channels_vec[3] != 0);
        
        int avg_val = channels_vec[4];
        if (avg_val >= 0 && avg_val <= 2) {
            opts.raw_channels.avg_mode = static_cast<AvgMode>(avg_val);
        } else {
            opts.raw_channels.avg_mode = AvgMode::Full; // Fallback to default
        }
    }

    return opts;
}

} // namespace DynaRange::Arguments::Parsing