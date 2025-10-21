// File: src/core/arguments/parsing/CliParser.cpp
/**
 * @file CliParser.cpp
 * @brief Implements the command-line argument parser.
 */
#include "CliParser.hpp"
#include "../Constants.hpp"
#include "../ArgumentsOptions.hpp"
#include "../../utils/PlatformUtils.hpp"
#include <CLI/CLI.hpp>
#include <libintl.h>

#define _(string) gettext(string)

namespace DynaRange::Arguments::Parsing {

// File: src/core/arguments/parsing/CliParser.cpp
std::map<std::string, std::any> CliParser::Parse(int argc, char* argv[], const std::map<std::string, ArgumentDescriptor>& descriptors)
{
    using namespace DynaRange::Arguments::Constants;
    std::map<std::string, std::any> values;

    CLI::App app { _("Calculates the dynamic range from a series of RAW images.") };
    auto fmt = app.get_formatter();
    fmt->column_width(35);

    ProgramOptions temp_opts; // Usado temporalmente por CLI11
    std::vector<double> temp_snr_thresholds;
    std::vector<int> temp_raw_channels;
    std::string temp_plot_format;
    std::vector<int> temp_plot_params;

    // --- Define all options ---
    auto chart_opt = app.add_option("-c,--chart", temp_opts.chart_params, descriptors.at(Chart).help_text)->expected(0,5); // Allow 0 args for default
    auto chart_colour_opt = app.add_option("-C,--chart-colour", temp_opts.chart_colour_params, descriptors.at(ChartColour).help_text)->expected(0, 4); // Allow 0 args for default
    auto chart_patches_opt = app.add_option("-M,--chart-patches", temp_opts.chart_patches, descriptors.at(ChartPatches).help_text)->expected(2);
    auto chart_coords_opt = app.add_option("-x,--chart-coords", temp_opts.chart_coords, descriptors.at(ChartCoords).help_text)->expected(8);
    auto input_opt = app.add_option("-i,--input-files", temp_opts.input_files, descriptors.at(InputFiles).help_text);
    auto black_file_opt = app.add_option("-b,--black-file", temp_opts.dark_file_path, descriptors.at(BlackFile).help_text)->check(CLI::ExistingFile);
    auto black_level_opt = app.add_option("-B,--black-level", temp_opts.dark_value, descriptors.at(BlackLevel).help_text);
    auto sat_file_opt = app.add_option("-s,--saturation-file", temp_opts.sat_file_path, descriptors.at(SaturationFile).help_text)->check(CLI::ExistingFile);
    auto sat_level_opt = app.add_option("-S,--saturation-level", temp_opts.saturation_value, descriptors.at(SaturationLevel).help_text);
    auto output_opt = app.add_option("-o,--output-file", temp_opts.output_filename, descriptors.at(OutputFile).help_text);
    auto snr_opt = app.add_option("-d,--snrthreshold-db", temp_snr_thresholds, descriptors.at(SnrThresholdDb).help_text);
    auto dr_norm_opt = app.add_option("-m,--drnormalization-mpx", temp_opts.dr_normalization_mpx, descriptors.at(DrNormalizationMpx).help_text);
    auto poly_fit_opt = app.add_option("-f,--poly-fit", temp_opts.poly_order, descriptors.at(PolyFit).help_text)
                            ->check(CLI::IsMember(std::vector<int>(std::begin(VALID_POLY_ORDERS), std::end(VALID_POLY_ORDERS))));
    auto patch_ratio_opt = app.add_option("-r,--patch-ratio", temp_opts.patch_ratio, descriptors.at(PatchRatio).help_text)->check(CLI::Range(0.0, 1.0));
    auto plot_format_opt = app.add_option("-p,--plot-format", temp_plot_format, descriptors.at(PlotFormat).help_text);
    auto plot_params_opt = app.add_option("-P,--plot-params", temp_plot_params, descriptors.at(PlotParams).help_text)->expected(4);
    auto print_patch_opt = app.add_option("-g,--print-patches", temp_opts.print_patch_filename, descriptors.at(PrintPatches).help_text)->expected(0, 1)->default_str("_USE_DEFAULT_PRINT_PATCHES_");
    auto raw_channel_opt = app.add_option("-w,--raw-channels", temp_raw_channels, descriptors.at(RawChannels).help_text)->expected(5);
    auto debug_opt = app.add_flag("-D,--debug", temp_opts.generate_full_debug, descriptors.at(FullDebug).help_text);


    // --- Single Parse Pass ---
    try {
        app.parse(argc, argv);
        if (chart_opt->count() == 0 && chart_colour_opt->count() == 0 && input_opt->count() == 0) {
            throw CLI::RequiredError(_("--input-files is required unless creating a chart with --chart or --chart-colour."));
        }
    } catch (const CLI::ParseError& e) {
        // Use standard streams for error output from CLI11's exit mechanism
        // exit() prints the error message and terminates.
        exit(app.exit(e));
    }

    // --- Store Parsed Values into the map ---
    if (chart_opt->count() > 0 || chart_colour_opt->count() > 0) {
        values[CreateChartMode] = true;
    }
    if (plot_format_opt->count() > 0 || plot_params_opt->count() > 0) {
        values[GeneratePlot] = true;
    }
    if (chart_opt->count() > 0) values[Chart] = temp_opts.chart_params;
    if (chart_colour_opt->count() > 0) values[ChartColour] = temp_opts.chart_colour_params;
    if (chart_patches_opt->count() > 0) values[ChartPatches] = temp_opts.chart_patches;
    if (chart_coords_opt->count() > 0) values[ChartCoords] = temp_opts.chart_coords;
    if (black_file_opt->count() > 0) {
        values[BlackFile] = temp_opts.dark_file_path;
        values[BlackLevelIsDefault] = false;
    }
    if (black_level_opt->count() > 0) {
        values[BlackLevel] = temp_opts.dark_value;
        values[BlackLevelIsDefault] = false;
    }
    if (sat_file_opt->count() > 0) {
        values[SaturationFile] = temp_opts.sat_file_path;
        values[SaturationLevelIsDefault] = false;
    }
    if (sat_level_opt->count() > 0) {
        values[SaturationLevel] = temp_opts.saturation_value;
        values[SaturationLevelIsDefault] = false;
    }
    if (raw_channel_opt->count() > 0) values[RawChannels] = temp_raw_channels;
    if (output_opt->count() > 0) values[OutputFile] = temp_opts.output_filename;
    if (dr_norm_opt->count() > 0) values[DrNormalizationMpx] = temp_opts.dr_normalization_mpx;
    if (poly_fit_opt->count() > 0) values[PolyFit] = temp_opts.poly_order;
    if (patch_ratio_opt->count() > 0) values[PatchRatio] = temp_opts.patch_ratio;
    if (plot_format_opt->count() > 0) values[PlotFormat] = temp_plot_format;
    if (plot_params_opt->count() > 0) values[PlotParams] = temp_plot_params;
    if (print_patch_opt->count() > 0) values[PrintPatches] = temp_opts.print_patch_filename;

    // --debug -D Full debug plotting
    // Read the actual boolean value parsed by CLI11 into temp_opts.generate_full_debug
    values[FullDebug] = temp_opts.generate_full_debug;

    values[InputFiles] = PlatformUtils::ExpandWildcards(temp_opts.input_files);
    if (snr_opt->count() > 0) {
        values[SnrThresholdDb] = temp_snr_thresholds;
        values[SnrThresholdIsDefault] = false;
    } else {
        // Ensure default values are populated if the option wasn't used
        // Get the default from the descriptor
        values[SnrThresholdDb] = std::any_cast<std::vector<double>>(descriptors.at(SnrThresholdDb).default_value);
        values[SnrThresholdIsDefault] = true;
    }


     // Populate missing values with defaults from descriptors
    for (const auto& [name, desc] : descriptors) {
        if (values.find(name) == values.end()) {
            values[name] = desc.default_value;
        }
    }


    return values;
}

} // namespace DynaRange::Arguments::Parsing