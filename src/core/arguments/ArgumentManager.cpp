// File: src/core/arguments/ArgumentManager.cpp
/**
 * @file ArgumentManager.cpp
 * @brief Implements the centralized argument management system.
 */
#include "ArgumentManager.hpp"
#include "ArgumentsOptions.hpp"
#include "../graphics/Constants.hpp"
#include "../utils/PlatformUtils.hpp"
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

void ArgumentManager::RegisterAllArguments() {
    if (m_is_registered) return;

    m_descriptors["black-level"] = {"black-level", "B", _("Camera RAW black level"), ArgType::Double, DEFAULT_BLACK_LEVEL};
    m_descriptors["black-file"] = {"black-file", "b", _("Totally dark RAW file ideally shot at base ISO"), ArgType::String, std::string("")};
    
    m_descriptors["saturation-level"] = {"saturation-level", "S", _("Camera RAW saturation level"), ArgType::Double, DEFAULT_SATURATION_LEVEL};
    m_descriptors["saturation-file"] = {"saturation-file", "s", _("Totally clipped RAW file ideally shot at base ISO"), ArgType::String, std::string("")};
    
    m_descriptors["input-files"] = {"input-files", "i", _("Input RAW files shot over the test chart ideally for every ISO"), ArgType::StringVector, std::vector<std::string>{}, false};
    m_descriptors["patch-ratio"] = {"patch-ratio", "r", _("Relative patch width/height used to compute signal and noise readings (default=0.5)"), ArgType::Double, DEFAULT_PATCH_RATIO, false, 0.0, 1.0};
    m_descriptors["snrthreshold-db"] = {"snrthreshold-db", "d", _("SNR threshold in dB for DR calculation (default=12dB (photo DR) plus 0dB (engineering DR))"), ArgType::Double, DEFAULT_SNR_THRESHOLD_DB};
    m_descriptors["drnormalization-mpx"] = {"drnormalization-mpx", "m", _("Number of Mpx for DR normalization (default=8Mpx)"), ArgType::Double, DEFAULT_DR_NORMALIZATION_MPX};
    m_descriptors["poly-fit"] = {"poly-fit", "f", _("Polynomic order (default=3) to fit the SNR curve"), ArgType::Int, DEFAULT_POLY_ORDER, false, 2, 3};
    m_descriptors["output-file"] = {"output-file", "o", _("Output CSV text file(s) with all results..."), ArgType::String, std::string(DEFAULT_OUTPUT_FILENAME)};
    m_descriptors["plot"] = {"plot", "p", _("Export SNR curves in PNG/PDF/SVG format..."), ArgType::Int, DEFAULT_PLOT_MODE};
    
    m_descriptors["print-patches"] = {"print-patches", "g", _("Saves a debug image ('chartpatches.png') with the patch overlay."), ArgType::String, std::string("")};
    
    m_descriptors["raw-channel"] = {"raw-channel", "w", _("Specify which RAW channels to analyze (R G1 G2 B AVG)"), ArgType::IntVector, std::vector<int>{0, 0, 0, 0, 1}};
    m_descriptors["generate-plot"] = {"generate-plot", "", "", ArgType::Flag, false};
    m_descriptors["plot-format"] = {"plot-format", "", "", ArgType::Int, DynaRange::Graphics::Constants::PlotOutputFormat::PNG};

    // Internal flags
    m_descriptors["snr-threshold-is-default"] = {"snr-threshold-is-default", "", "", ArgType::Flag, true};
    m_descriptors["black-level-is-default"] = {"black-level-is-default", "", "", ArgType::Flag, true};
    m_descriptors["saturation-level-is-default"] = {"saturation-level-is-default", "", "", ArgType::Flag, true};
    
    // Chart arguments
    m_descriptors["chart"] = {"chart", "c", _("specify format of test chart (default DIMX=1920, W=3, H=2)"), ArgType::IntVector, std::vector<int>()};
    m_descriptors["chart-colour"] = {"chart-colour", "C", _("Create test chart in PNG format ranging colours..."), ArgType::StringVector, std::vector<std::string>()};
    m_descriptors["create-chart-mode"] = {"create-chart-mode", "", "", ArgType::Flag, false};
    m_descriptors["chart-coords"] = {"chart-coords", "x", _("Test chart defined by 4 corners: tl, bl, br, tr"), ArgType::DoubleVector, std::vector<double>()};
    m_descriptors["chart-patches"] = {"chart-patches", "M", _("Specify number of patches over rows (M) and columns (N) (default M=4, N=6)"), ArgType::IntVector, std::vector<int>()};
    m_is_registered = true;
}

void ArgumentManager::ParseCli(int argc, char* argv[]) {
    CLI::App app{_("Calculates the dynamic range from a series of RAW images.")};
    auto fmt = app.get_formatter();
    fmt->column_width(35);

    ProgramOptions temp_opts;
    std::vector<double> temp_snr_thresholds;
    std::vector<int> temp_raw_channels;
    std::vector<std::string> plot_params;
    int plot_command_mode = 1;

    // --- Define all options ---
    auto chart_opt = app.add_option("-c,--chart", temp_opts.chart_params, m_descriptors.at("chart").help_text)->expected(5);
    auto chart_colour_opt = app.add_option("-C,--chart-colour", temp_opts.chart_colour_params, m_descriptors.at("chart-colour").help_text)->expected(0, 4);
    auto chart_patches_opt = app.add_option("-M,--chart-patches", temp_opts.chart_patches, m_descriptors.at("chart-patches").help_text)->expected(2);
    auto chart_coords_opt = app.add_option("-x,--chart-coords", temp_opts.chart_coords, m_descriptors.at("chart-coords").help_text)->expected(8);
    auto input_opt = app.add_option("-i,--input-files", temp_opts.input_files, m_descriptors.at("input-files").help_text);
    
    auto black_file_opt = app.add_option("-b,--black-file", temp_opts.dark_file_path, m_descriptors.at("black-file").help_text)->check(CLI::ExistingFile);
    auto black_level_opt = app.add_option("-B,--black-level", temp_opts.dark_value, m_descriptors.at("black-level").help_text);
    auto sat_file_opt = app.add_option("-s,--saturation-file", temp_opts.sat_file_path, m_descriptors.at("saturation-file").help_text)->check(CLI::ExistingFile);
    auto sat_level_opt = app.add_option("-S,--saturation-level", temp_opts.saturation_value, m_descriptors.at("saturation-level").help_text);

    auto output_opt = app.add_option("-o,--output-file", temp_opts.output_filename, m_descriptors.at("output-file").help_text);
    auto snr_opt = app.add_option("-d,--snrthreshold-db", temp_snr_thresholds, m_descriptors.at("snrthreshold-db").help_text);
    auto dr_norm_opt = app.add_option("-m,--drnormalization-mpx", temp_opts.dr_normalization_mpx, m_descriptors.at("drnormalization-mpx").help_text);
    auto poly_fit_opt = app.add_option("-f,--poly-fit", temp_opts.poly_order, m_descriptors.at("poly-fit").help_text)
                            ->check(CLI::IsMember(std::vector<int>(std::begin(VALID_POLY_ORDERS), std::end(VALID_POLY_ORDERS))));
    auto patch_ratio_opt = app.add_option("-r,--patch-ratio", temp_opts.patch_ratio, m_descriptors.at("patch-ratio").help_text)->check(CLI::Range(0.0, 1.0));
    
    auto plot_opt = app.add_option("-p,--plot", plot_params, _("Export SNR curves. Can specify [FORMAT] or [FILENAME FORMAT]."))->expected(0,2)->trigger_on_parse();
    app.add_option("--plot-cmd", plot_command_mode, _("Set plot command mode (0-3)."))->check(CLI::Range(0, 3));

    auto print_patch_opt = app.add_option("-g,--print-patches", temp_opts.print_patch_filename, m_descriptors.at("print-patches").help_text)
                                ->expected(0,1)
                                ->default_str("chartpatches.png");
    auto raw_channel_opt = app.add_option("-w,--raw-channel", temp_raw_channels, m_descriptors.at("raw-channel").help_text)->expected(5);

    // --- Single Parse Pass ---
    try {
        app.parse(argc, argv);
        if (chart_opt->count() == 0 && chart_colour_opt->count() == 0 && input_opt->count() == 0) {
            throw CLI::RequiredError(_("--input-files is required unless creating a chart with --chart or --chart-colour."));
        }
    } catch (const CLI::ParseError &e) {
        exit(app.exit(e));
    }
    
    // --- Store Parsed Values ---
    if (chart_opt->count() > 0 || chart_colour_opt->count() > 0) {
        m_values["create-chart-mode"] = true;
    }
    
    if (chart_opt->count() > 0) m_values["chart"] = temp_opts.chart_params;
    if (chart_colour_opt->count() > 0) m_values["chart-colour"] = temp_opts.chart_colour_params;
    if (chart_patches_opt->count() > 0) m_values["chart-patches"] = temp_opts.chart_patches;
    if (chart_coords_opt->count() > 0) m_values["chart-coords"] = temp_opts.chart_coords;

    if (black_file_opt->count() > 0) {
        m_values["black-file"] = temp_opts.dark_file_path;
        m_values["black-level-is-default"] = false;
    }
    if (black_level_opt->count() > 0) {
        m_values["black-level"] = temp_opts.dark_value;
        m_values["black-level-is-default"] = false;
    }

    if (sat_file_opt->count() > 0) {
        m_values["saturation-file"] = temp_opts.sat_file_path;
        m_values["saturation-level-is-default"] = false;
    }
    if (sat_level_opt->count() > 0) {
        m_values["saturation-level"] = temp_opts.saturation_value;
        m_values["saturation-level-is-default"] = false;
    }
    
    if (raw_channel_opt->count() > 0) {
        m_values["raw-channel"] = temp_raw_channels;
    }
    
    if (output_opt->count() > 0) m_values["output-file"] = temp_opts.output_filename;
    if (dr_norm_opt->count() > 0) m_values["drnormalization-mpx"] = temp_opts.dr_normalization_mpx;
    if (poly_fit_opt->count() > 0) m_values["poly-fit"] = temp_opts.poly_order;
    if (patch_ratio_opt->count() > 0) m_values["patch-ratio"] = temp_opts.patch_ratio;
    
    if (plot_opt->count() > 0) {
        m_values["generate-plot"] = true;
        std::string format_str = "PNG"; // Default format
        std::string filename = "";
        
        // Default to mode 1 if no integer is found.
        plot_command_mode = 1; 

        for (const auto& param : plot_params) {
            std::string upper_param = param;
            std::transform(upper_param.begin(), upper_param.end(), upper_param.begin(), ::toupper);

            // Check if the parameter is a valid integer mode (1, 2, or 3)
            if (param == "1" || param == "2" || param == "3") {
                try {
                    plot_command_mode = std::stoi(param);
                } catch (...) { /* Ignore conversion errors, will fallback to default */ }
            } else if (upper_param == "PNG" || upper_param == "SVG") { // PDF removed
                format_str = upper_param;
            } else {
                filename = param;
            }
        }
        
        // Store the correctly parsed command mode.
        m_values["plot"] = plot_command_mode;
        
        if (filename.empty()) {
            std::string ext = (format_str == "SVG") ? ".svg" : ".png"; // PDF removed
            filename = "snrcurves" + ext;
        }
        
        if (format_str == "SVG") m_values["plot-format"] = DynaRange::Graphics::Constants::PlotOutputFormat::SVG;
        else m_values["plot-format"] = DynaRange::Graphics::Constants::PlotOutputFormat::PNG; // PDF logic removed
    }

    if (print_patch_opt->count() > 0) m_values["print-patches"] = temp_opts.print_patch_filename;
    
    m_values["input-files"] = PlatformUtils::ExpandWildcards(temp_opts.input_files);
    if (snr_opt->count() > 0) {
        m_values["snrthreshold-db"] = temp_snr_thresholds;
        // This should handle multiple values.
        m_values["snr-threshold-is-default"] = false;
    }
}

ProgramOptions ArgumentManager::ToProgramOptions() {
    ProgramOptions opts;
    
    opts.create_chart_mode = Get<bool>("create-chart-mode");
    opts.chart_params = Get<std::vector<int>>("chart");
    opts.chart_colour_params = Get<std::vector<std::string>>("chart-colour");
    opts.chart_coords = Get<std::vector<double>>("chart-coords");
    opts.chart_patches = Get<std::vector<int>>("chart-patches");    
    opts.dark_value = Get<double>("black-level");
    opts.saturation_value = Get<double>("saturation-level");
    opts.dark_file_path = Get<std::string>("black-file");
    opts.sat_file_path = Get<std::string>("saturation-file");
    opts.output_filename = Get<std::string>("output-file");
    opts.input_files = Get<std::vector<std::string>>("input-files");
    opts.poly_order = Get<int>("poly-fit");
    opts.dr_normalization_mpx = Get<double>("drnormalization-mpx");
    opts.patch_ratio = Get<double>("patch-ratio");
    
    // Populate new plot-related members
    opts.generate_plot = Get<bool>("generate-plot");
    if (opts.generate_plot) {
         opts.plot_format = Get<DynaRange::Graphics::Constants::PlotOutputFormat>("plot-format");
         opts.plot_command_mode = Get<int>("plot");
    }

    opts.print_patch_filename = Get<std::string>("print-patches");
    opts.black_level_is_default = Get<bool>("black-level-is-default");
    opts.saturation_level_is_default = Get<bool>("saturation-level-is-default");

    if (Get<bool>("snr-threshold-is-default")) {
         opts.snr_thresholds_db = {12.0, 0.0};
    } else {
         opts.snr_thresholds_db = { Get<double>("snrthreshold-db") };
    }

    auto channels_vec = Get<std::vector<int>>("raw-channel");
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