// File: src/core/arguments/ArgumentManager.cpp
/**
 * @file ArgumentManager.cpp
 * @brief Implements the centralized argument management system.
 */
#include "ArgumentManager.hpp"
#include "Constants.hpp"
#include "../graphics/Constants.hpp"
#include "../utils/PlatformUtils.hpp"
#include "ArgumentsOptions.hpp"
#include <CLI/CLI.hpp>
#include <libintl.h>

#define _(string) gettext(string)

// --- ArgumentManager Implementation ---

ArgumentManager& ArgumentManager::Instance()
{
    static ArgumentManager instance;
    return instance;
}

ArgumentManager::ArgumentManager()
{
    RegisterAllArguments();
    // Populate the values map with defaults immediately upon creation.
    for (const auto& [name, desc] : m_descriptors) {
        m_values[name] = desc.default_value;
    }
}

void ArgumentManager::RegisterAllArguments()
{
    if (m_is_registered)
        return;

    using namespace DynaRange::Arguments::Constants;

    m_descriptors[BlackLevel] = { BlackLevel, "B", _("Camera RAW black level"), ArgType::Double, DEFAULT_BLACK_LEVEL };
    m_descriptors[BlackFile] = { BlackFile, "b", _("Totally dark RAW file ideally shot at base ISO"), ArgType::String, std::string("") };
    m_descriptors[SaturationLevel] = { SaturationLevel, "S", _("Camera RAW saturation level"), ArgType::Double, DEFAULT_SATURATION_LEVEL };
    m_descriptors[SaturationFile] = { SaturationFile, "s", _("Totally clipped RAW file ideally shot at base ISO"), ArgType::String, std::string("") };
    m_descriptors[InputFiles] = { InputFiles, "i", _("Input RAW files shot over the test chart ideally for every ISO"), ArgType::StringVector, std::vector<std::string> {}, false };
    m_descriptors[PatchRatio] = { PatchRatio, "r", _("Relative patch width/height used to compute signal and noise readings (default=0.5)"), ArgType::Double, DEFAULT_PATCH_RATIO, false, 0.0, 1.0 };
    m_descriptors[SnrThresholdDb] = { SnrThresholdDb, "d", _("SNR threshold(s) list in dB for DR calculation (default=0 12 being 0dB=\"Engineering DR\" and 12dB=\"Photographic DR\")"), ArgType::DoubleVector, std::vector<double> { 12.0, 0.0 } };
    m_descriptors[DrNormalizationMpx] = { DrNormalizationMpx, "m", _("Number of Mpx for DR normalization (default=8Mpx, no normalization=per pixel DR=0Mpx)"), ArgType::Double, DEFAULT_DR_NORMALIZATION_MPX };
    m_descriptors[PolyFit] = { PolyFit, "f", _("Polynomic order to fit the SNR curve (default=3)"), ArgType::Int, DEFAULT_POLY_ORDER, false, 2, 3 };
    m_descriptors[OutputFile] = { OutputFile, "o", _("Output CSV text file(s) with all results..."), ArgType::String, std::string(DEFAULT_OUTPUT_FILENAME) };
    m_descriptors[PlotFormat] = { PlotFormat, "p", _("Export SNR curves plot in PNG/SVG format (default format=PNG)"), ArgType::String, std::string("PNG") };
    m_descriptors[PlotParams] = { PlotParams, "P", _("Export SNR curves with SCL 1-3 info (default=1 1 1 3)"), ArgType::IntVector, std::vector<int> { 1, 1, 1, 3 } };
    m_descriptors[PrintPatches] = { PrintPatches, "g", _("Save keystone/ETTR/gamma corrected test chart in PNG format indicating the grid of patches used for all calculations (default=\"printpatches.png\")"), ArgType::String, std::string("") };
    m_descriptors[RawChannels] = { RawChannels, "w", _("Specify with 0/1 boolean values for which RAW channel(s) the calculations (SNR curves, DR) will be carried out (default=0 0 0 0 1)"), ArgType::IntVector, std::vector<int> { 0, 0, 0, 0, 1 } };
    m_descriptors[GeneratePlot] = { GeneratePlot, "", "", ArgType::Flag, false };

    // Internal flags
    m_descriptors[SnrThresholdIsDefault] = { SnrThresholdIsDefault, "", "", ArgType::Flag, true };
    m_descriptors[BlackLevelIsDefault] = { BlackLevelIsDefault, "", "", ArgType::Flag, true };
    m_descriptors[SaturationLevelIsDefault] = { SaturationLevelIsDefault, "", "", ArgType::Flag, true };

    // Chart arguments
    m_descriptors[Chart] = { Chart, "c", _("specify format of test chart (default DIMX=1920, W=3, H=2)"), ArgType::IntVector, std::vector<int>() };
    m_descriptors[ChartColour] = { ChartColour, "C", _("Create test chart in PNG format ranging colours..."), ArgType::StringVector, std::vector<std::string>() };
    m_descriptors[CreateChartMode] = { CreateChartMode, "", "", ArgType::Flag, false };
    m_descriptors[ChartCoords] = { ChartCoords, "x", _("Test chart defined by 4 corners: tl, bl, br, tr"), ArgType::DoubleVector, std::vector<double>() };
    m_descriptors[ChartPatches] = { ChartPatches, "M", _("Specify number of patches over rows (M) and columns (N) (default M=4, N=6)"), ArgType::IntVector, std::vector<int>() };
    m_is_registered = true;
}

void ArgumentManager::ParseCli(int argc, char* argv[])
{
    using namespace DynaRange::Arguments::Constants;
    CLI::App app { _("Calculates the dynamic range from a series of RAW images.") };
    auto fmt = app.get_formatter();
    fmt->column_width(35);

    ProgramOptions temp_opts;
    std::vector<double> temp_snr_thresholds;
    std::vector<int> temp_raw_channels;
    std::string temp_plot_format;
    std::vector<int> temp_plot_params;

    // --- Define all options ---
    auto chart_opt = app.add_option("-c,--chart", temp_opts.chart_params, m_descriptors.at(Chart).help_text)->expected(5);
    auto chart_colour_opt = app.add_option("-C,--chart-colour", temp_opts.chart_colour_params, m_descriptors.at(ChartColour).help_text)->expected(0, 4);
    auto chart_patches_opt = app.add_option("-M,--chart-patches", temp_opts.chart_patches, m_descriptors.at(ChartPatches).help_text)->expected(2);
    auto chart_coords_opt = app.add_option("-x,--chart-coords", temp_opts.chart_coords, m_descriptors.at(ChartCoords).help_text)->expected(8);
    auto input_opt = app.add_option("-i,--input-files", temp_opts.input_files, m_descriptors.at(InputFiles).help_text);

    auto black_file_opt = app.add_option("-b,--black-file", temp_opts.dark_file_path, m_descriptors.at(BlackFile).help_text)->check(CLI::ExistingFile);
    auto black_level_opt = app.add_option("-B,--black-level", temp_opts.dark_value, m_descriptors.at(BlackLevel).help_text);
    auto sat_file_opt = app.add_option("-s,--saturation-file", temp_opts.sat_file_path, m_descriptors.at(SaturationFile).help_text)->check(CLI::ExistingFile);
    auto sat_level_opt = app.add_option("-S,--saturation-level", temp_opts.saturation_value, m_descriptors.at(SaturationLevel).help_text);

    auto output_opt = app.add_option("-o,--output-file", temp_opts.output_filename, m_descriptors.at(OutputFile).help_text);
    auto snr_opt = app.add_option("-d,--snrthreshold-db", temp_snr_thresholds, m_descriptors.at(SnrThresholdDb).help_text);
    auto dr_norm_opt = app.add_option("-m,--drnormalization-mpx", temp_opts.dr_normalization_mpx, m_descriptors.at(DrNormalizationMpx).help_text);
    auto poly_fit_opt = app.add_option("-f,--poly-fit", temp_opts.poly_order, m_descriptors.at(PolyFit).help_text)
                            ->check(CLI::IsMember(std::vector<int>(std::begin(VALID_POLY_ORDERS), std::end(VALID_POLY_ORDERS))));
    auto patch_ratio_opt = app.add_option("-r,--patch-ratio", temp_opts.patch_ratio, m_descriptors.at(PatchRatio).help_text)->check(CLI::Range(0.0, 1.0));

    auto plot_format_opt = app.add_option("-p,--plot-format", temp_plot_format, m_descriptors.at(PlotFormat).help_text);
    auto plot_params_opt = app.add_option("-P,--plot-params", temp_plot_params, m_descriptors.at(PlotParams).help_text)->expected(4);

    auto print_patch_opt = app.add_option("-g,--print-patches", temp_opts.print_patch_filename, m_descriptors.at(PrintPatches).help_text)->expected(0, 1)->default_str("chartpatches.png");
    auto raw_channel_opt = app.add_option("-w,--raw-channels", temp_raw_channels, m_descriptors.at(RawChannels).help_text)->expected(5);

    // --- Single Parse Pass ---
    try {
        app.parse(argc, argv);
        if (chart_opt->count() == 0 && chart_colour_opt->count() == 0 && input_opt->count() == 0) {
            throw CLI::RequiredError(_("--input-files is required unless creating a chart with --chart or --chart-colour."));
        }
    } catch (const CLI::ParseError& e) {
        exit(app.exit(e));
    }

    // --- Store Parsed Values ---
    if (chart_opt->count() > 0 || chart_colour_opt->count() > 0) {
        m_values[CreateChartMode] = true;
    }

    if (plot_format_opt->count() > 0 || plot_params_opt->count() > 0) {
        m_values[GeneratePlot] = true;
    }

    if (chart_opt->count() > 0)
        m_values[Chart] = temp_opts.chart_params;
    if (chart_colour_opt->count() > 0)
        m_values[ChartColour] = temp_opts.chart_colour_params;
    if (chart_patches_opt->count() > 0)
        m_values[ChartPatches] = temp_opts.chart_patches;
    if (chart_coords_opt->count() > 0)
        m_values[ChartCoords] = temp_opts.chart_coords;

    if (black_file_opt->count() > 0) {
        m_values[BlackFile] = temp_opts.dark_file_path;
        m_values[BlackLevelIsDefault] = false;
    }
    if (black_level_opt->count() > 0) {
        m_values[BlackLevel] = temp_opts.dark_value;
        m_values[BlackLevelIsDefault] = false;
    }

    if (sat_file_opt->count() > 0) {
        m_values[SaturationFile] = temp_opts.sat_file_path;
        m_values[SaturationLevelIsDefault] = false;
    }
    if (sat_level_opt->count() > 0) {
        m_values[SaturationLevel] = temp_opts.saturation_value;
        m_values[SaturationLevelIsDefault] = false;
    }

    if (raw_channel_opt->count() > 0) {
        m_values[RawChannels] = temp_raw_channels;
    }

    if (output_opt->count() > 0)
        m_values[OutputFile] = temp_opts.output_filename;
    if (dr_norm_opt->count() > 0)
        m_values[DrNormalizationMpx] = temp_opts.dr_normalization_mpx;
    if (poly_fit_opt->count() > 0)
        m_values[PolyFit] = temp_opts.poly_order;
    if (patch_ratio_opt->count() > 0)
        m_values[PatchRatio] = temp_opts.patch_ratio;

    if (plot_format_opt->count() > 0)
        m_values[PlotFormat] = temp_plot_format;
    if (plot_params_opt->count() > 0)
        m_values[PlotParams] = temp_plot_params;

    if (print_patch_opt->count() > 0)
        m_values[PrintPatches] = temp_opts.print_patch_filename;
    m_values[InputFiles] = PlatformUtils::ExpandWildcards(temp_opts.input_files);
    if (snr_opt->count() > 0) {
        m_values[SnrThresholdDb] = temp_snr_thresholds;
        m_values[SnrThresholdIsDefault] = false;
    }
}

ProgramOptions ArgumentManager::ToProgramOptions()
{
    using namespace DynaRange::Arguments::Constants;
    ProgramOptions opts;

    opts.create_chart_mode = Get<bool>(CreateChartMode);
    opts.chart_params = Get<std::vector<int>>(Chart);
    opts.chart_colour_params = Get<std::vector<std::string>>(ChartColour);
    opts.chart_coords = Get<std::vector<double>>(ChartCoords);
    opts.chart_patches = Get<std::vector<int>>(ChartPatches);
    opts.dark_value = Get<double>(BlackLevel);
    opts.saturation_value = Get<double>(SaturationLevel);
    opts.dark_file_path = Get<std::string>(BlackFile);
    opts.sat_file_path = Get<std::string>(SaturationFile);
    opts.output_filename = Get<std::string>(OutputFile);
    opts.input_files = Get<std::vector<std::string>>(InputFiles);
    opts.poly_order = Get<int>(PolyFit);
    opts.dr_normalization_mpx = Get<double>(DrNormalizationMpx);
    opts.patch_ratio = Get<double>(PatchRatio);

    opts.generate_plot = Get<bool>(GeneratePlot);
    if (opts.generate_plot) {
        std::string format_str = Get<std::string>(PlotFormat);
        std::transform(format_str.begin(), format_str.end(), format_str.begin(), ::toupper);
        if (format_str == "SVG")
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::SVG;
        else if (format_str == "PDF")
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PDF;
        else
            opts.plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PNG;

        auto params_vec = Get<std::vector<int>>(PlotParams);
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

    opts.print_patch_filename = Get<std::string>(PrintPatches);
    opts.black_level_is_default = Get<bool>(BlackLevelIsDefault);
    opts.saturation_level_is_default = Get<bool>(SaturationLevelIsDefault);
    
    if (Get<bool>(SnrThresholdIsDefault)) {
        opts.snr_thresholds_db = { 12.0, 0.0 };
    } else {
        opts.snr_thresholds_db = Get<std::vector<double>>(SnrThresholdDb);
    }

    auto channels_vec = Get<std::vector<int>>(RawChannels);
    if (channels_vec.size() == 5) {
        opts.raw_channels.R = (channels_vec[0] != 0);
        opts.raw_channels.G1 = (channels_vec[1] != 0);
        opts.raw_channels.G2 = (channels_vec[2] != 0);
        opts.raw_channels.B = (channels_vec[3] != 0);
        opts.raw_channels.AVG = (channels_vec[4] != 0);
    }

    return opts;
}

void ArgumentManager::Set(const std::string& long_name, std::any value)
{
    if (m_descriptors.count(long_name)) {
        m_values[long_name] = std::move(value);
    }
}