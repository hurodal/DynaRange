// File: src/core/arguments/ArgumentManager.cpp
/**
 * @file ArgumentManager.cpp
 * @brief Implements the centralized argument management system.
 */
#include "ArgumentManager.hpp"
#include "../graphics/ChartGenerator.hpp"
#include <CLI/CLI.hpp>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <libintl.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define _(string) gettext(string)
namespace fs = std::filesystem;

// --- Windows Wildcard Expansion Helpers ---
#ifdef _WIN32
namespace { // Anonymous namespace for internal helpers

// Expands a single file pattern on Windows.
void expand_single_wildcard(const std::string& pattern, std::vector<std::string>& expanded_files) {
    WIN32_FIND_DATAA find_data;
    HANDLE h_find = FindFirstFileA(pattern.c_str(), &find_data);
    if (h_find != INVALID_HANDLE_VALUE) {
        fs::path pattern_path(pattern);
        fs::path parent_dir = pattern_path.parent_path();
        do {
            // Ensure we only add files, not directories.
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                expanded_files.push_back((parent_dir / find_data.cFileName).string());
            }
        } while (FindNextFileA(h_find, &find_data) != 0);
        FindClose(h_find);
    }
}

// Processes a list of file arguments and expands any that contain wildcards.
std::vector<std::string> expand_wildcards_on_windows(const std::vector<std::string>& files) {
    std::vector<std::string> result_files;
    for (const auto& file_arg : files) {
        // An argument is a pattern if it contains '*' or '?'.
        if (file_arg.find_first_of("*?") != std::string::npos) {
            expand_single_wildcard(file_arg, result_files);
        } else {
            // If it has no wildcards, add it directly.
            result_files.push_back(file_arg);
        }
    }
    return result_files;
}
} // end anonymous namespace
#endif

// --- ArgumentManager Implementation ---

ArgumentManager& ArgumentManager::Instance() {
    static ArgumentManager instance;
    return instance;
}

ArgumentManager::ArgumentManager() {
    RegisterAllArguments();
    // Populate the values map with defaults immediately upon creation.
    for (const auto& [name, desc] : m_descriptors) {
        m_values[name] = desc.default_value;
    }
}

void ArgumentManager::RegisterAllArguments() {
    if (m_is_registered) return;

    m_descriptors["black-level"] = {"black-level", "b", _("Camera RAW black level"), ArgType::Double, 0.0};
    m_descriptors["black-file"] = {"black-file", "B", _("Totally dark RAW file ideally shot at base ISO"), ArgType::String, std::string("")};
    m_descriptors["saturation-level"] = {"saturation-level", "s", _("Camera RAW saturation level"), ArgType::Double, 16383.0};
    m_descriptors["saturation-file"] = {"saturation-file", "S", _("Totally clipped RAW file ideally shot at base ISO"), ArgType::String, std::string("")};
    m_descriptors["input-files"] = {"input-files", "i", _("Input RAW files shot over the test chart ideally for every ISO"), ArgType::StringVector, std::vector<std::string>{}, false};
    m_descriptors["patch-ratio"] = {"patch-ratio", "r", _("Relative patch width/height used to compute signal and noise readings (default=0.5)"), ArgType::Double, 0.5, false, 0.0, 1.0};
    m_descriptors["snrthreshold-db"] = {"snrthreshold-db", "d", _("SNR threshold in dB for DR calculation (default=12dB (photo DR) plus 0dB (engineering DR))"), ArgType::Double, 12.0};
    m_descriptors["drnormalization-mpx"] = {"drnormalization-mpx", "m", _("Number of Mpx for DR normalization (default=8Mpx)"), ArgType::Double, 8.0};
    m_descriptors["poly-fit"] = {"poly-fit", "f", _("Polynomic order (default=3) to fit the SNR curve"), ArgType::Int, 3, false, 2, 3};
    m_descriptors["output-file"] = {"output-file", "o", _("Output CSV text file(s) with all results: black level, sat level, SNR samples, DR values, fitting params (default=\"DR_results.csv\")"), ArgType::String, std::string("DR_results.csv")};
    m_descriptors["plot"] = {"plot", "p", _("Export SNR curves in PNG format (0=no, 1=no cmd, 2=short cmd, 3=long cmd)"), ArgType::Int, 0, false, 0, 3};
    m_descriptors["snr-threshold-is-default"] = {"snr-threshold-is-default", "", "", ArgType::Flag, true};
    
    // Descriptor for the chart argument, accepting optional string parameters
    m_descriptors["chart"] = {
        "chart", "c",
        _("Create test chart in PNG format with optional params <R G B invgamma>"),
        ArgType::StringVector, // Use StringVector to accept 0 to 4 optional parameters
        std::vector<std::string>()
    };
    // Internal use only to carry parsed chart parameters
    m_descriptors["chart-params"] = {"chart-params", "", "", ArgType::StringVector, std::vector<std::string>()};

    m_is_registered = true;
}

void ArgumentManager::ParseCli(int argc, char* argv[]) {
    CLI::App app{_("Calculates the dynamic range from a series of RAW images.")};
    auto fmt = app.get_formatter();
    fmt->column_width(35);

    ProgramOptions temp_opts;
    std::vector<double> temp_snr_thresholds;
    
    // Bind variables and capture the Option pointers to check if they were used.
    auto chart_opt = app.add_option("-c,--chart", temp_opts.chart_params, m_descriptors.at("chart").help_text)
        ->expected(0, 4); // 0 to 4 optional parameters
    auto black_file_opt = app.add_option("-B,--black-file", temp_opts.dark_file_path, m_descriptors.at("black-file").help_text)->check(CLI::ExistingFile);
    auto black_level_opt = app.add_option("-b,--black-level", temp_opts.dark_value, m_descriptors.at("black-level").help_text);
    auto sat_file_opt = app.add_option("-S,--saturation-file", temp_opts.sat_file_path, m_descriptors.at("saturation-file").help_text)->check(CLI::ExistingFile);
    auto sat_level_opt = app.add_option("-s,--saturation-level", temp_opts.saturation_value, m_descriptors.at("saturation-level").help_text);
    auto input_opt = app.add_option("-i,--input-files", temp_opts.input_files, m_descriptors.at("input-files").help_text);
    auto output_opt = app.add_option("-o,--output-file", temp_opts.output_filename, m_descriptors.at("output-file").help_text);
    auto snr_opt = app.add_option("-d,--snrthreshold-db", temp_snr_thresholds, m_descriptors.at("snrthreshold-db").help_text);
    auto dr_norm_opt = app.add_option("-m,--drnormalization-mpx", temp_opts.dr_normalization_mpx, m_descriptors.at("drnormalization-mpx").help_text);
    auto poly_fit_opt = app.add_option("-f,--poly-fit", temp_opts.poly_order, m_descriptors.at("poly-fit").help_text)->check(CLI::Range(2, 3));
    auto patch_ratio_opt = app.add_option("-r,--patch-ratio", temp_opts.patch_ratio, m_descriptors.at("patch-ratio").help_text)->check(CLI::Range(0.0, 1.0));
    auto plot_opt = app.add_option("-p,--plot", temp_opts.plot_mode, m_descriptors.at("plot").help_text)->check(CLI::Range(0, 3));

    // First parse to see if --chart is present
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        exit(app.exit(e));
    }

    #ifdef _WIN32
        temp_opts.input_files = expand_wildcards_on_windows(temp_opts.input_files);
    #endif

    // Set the create_chart_mode flag if the --chart option was used at all.
    if (chart_opt->count() > 0) {
        temp_opts.create_chart_mode = true;
    }

    // If --chart is not used, input files are required.
    if (!temp_opts.create_chart_mode) {
        input_opt->required();
    }
    
    // Re-parse with the conditional requirement set.
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        exit(app.exit(e));
    }

    // --- Update internal values map ---
    if (chart_opt->count() > 0) {
        m_values["chart"] = temp_opts.create_chart_mode;
        m_values["chart-params"] = temp_opts.chart_params;
    }
    if (black_file_opt->count() > 0) m_values["black-file"] = temp_opts.dark_file_path;
    if (black_level_opt->count() > 0) m_values["black-level"] = temp_opts.dark_value;
    if (sat_file_opt->count() > 0) m_values["saturation-file"] = temp_opts.sat_file_path;
    if (sat_level_opt->count() > 0) m_values["saturation-level"] = temp_opts.saturation_value;
    if (output_opt->count() > 0) m_values["output-file"] = temp_opts.output_filename;
    if (dr_norm_opt->count() > 0) m_values["drnormalization-mpx"] = temp_opts.dr_normalization_mpx;
    if (poly_fit_opt->count() > 0) m_values["poly-fit"] = temp_opts.poly_order;
    if (patch_ratio_opt->count() > 0) m_values["patch-ratio"] = temp_opts.patch_ratio;
    if (plot_opt->count() > 0) m_values["plot"] = temp_opts.plot_mode;
    
    m_values["input-files"] = temp_opts.input_files;
    
    if (snr_opt->count() > 0) {
        m_values["snrthreshold-db"] = temp_snr_thresholds[0];
        m_values["snr-threshold-is-default"] = false;
    }
}

void ArgumentManager::Set(const std::string& long_name, std::any value) {
    if (m_descriptors.count(long_name)) {
        m_values[long_name] = std::move(value);
    }
}

ProgramOptions ArgumentManager::ToProgramOptions() {
    ProgramOptions opts;
    
    opts.create_chart_mode = Get<bool>("chart");
    opts.dark_value = Get<double>("black-level");
    opts.saturation_value = Get<double>("saturation-level");
    opts.dark_file_path = Get<std::string>("black-file");
    opts.sat_file_path = Get<std::string>("saturation-file");
    opts.output_filename = Get<std::string>("output-file");
    opts.input_files = Get<std::vector<std::string>>("input-files");
    opts.poly_order = Get<int>("poly-fit");
    opts.dr_normalization_mpx = Get<double>("drnormalization-mpx");
    opts.patch_ratio = Get<double>("patch-ratio");
    opts.plot_mode = Get<int>("plot");

    if (Get<bool>("snr-threshold-is-default")) {
         opts.snr_thresholds_db = {12.0, 0.0};
    } else {
         opts.snr_thresholds_db = { Get<double>("snrthreshold-db") };
    }

    // This line is removed as this logic was moved to InitializeAnalysis()
    // opts.generated_command = GenerateCommand(CommandFormat::Plot);

    return opts;
}

std::string ArgumentManager::GenerateCommand(CommandFormat format) {
    std::stringstream command_ss;
    command_ss << "rango";

    // Helper lambda to dynamically add arguments based on format
    auto add_arg = [&](const std::string& name) {
        const auto& desc = m_descriptors.at(name);
        const auto& value = m_values.at(name);
        // Short names are used only for the PlotShort format. All others use long names.
        bool use_short = (format == CommandFormat::PlotShort);
        
        command_ss << " " << (use_short ? "-" + desc.short_name : "--" + desc.long_name);

        // For flags, no value is added. For others, it is.
        if (desc.type != ArgType::Flag) {
            command_ss << " ";
            if (desc.type == ArgType::String) {
                std::string path_str = std::any_cast<std::string>(value);
                // Use full paths for 'Full' and 'GuiPreview' formats.
                bool use_full_path = (format == CommandFormat::Full || format == CommandFormat::GuiPreview);
                std::string final_path = use_full_path ? path_str : fs::path(path_str).filename().string();
                command_ss << "\"" << final_path << "\"";
            } else if (desc.type == ArgType::Double) {
                command_ss << std::fixed << std::setprecision(2) << std::any_cast<double>(value);
            } else if (desc.type == ArgType::Int) {
                command_ss << std::any_cast<int>(value);
            }
        }
    };

    // Logic to add arguments based on their state
    if (!Get<std::string>("black-file").empty()) {
        add_arg("black-file");
    } else {
        add_arg("black-level");
    }

    if (!Get<std::string>("saturation-file").empty()) {
        add_arg("saturation-file");
    } else {
        add_arg("saturation-level");
    }
    
    // The output file is only relevant for the full, non-plot command
    if (format == CommandFormat::Full) {
        add_arg("output-file");
    }

    if (!Get<bool>("snr-threshold-is-default")) {
        add_arg("snrthreshold-db");
    }

    add_arg("drnormalization-mpx");
    add_arg("poly-fit");
    add_arg("patch-ratio");
    add_arg("plot");

    // The 'input-files' argument is now only added for the Full and GuiPreview formats
    // to keep the command printed on the plot images clean.
    if (format == CommandFormat::Full || format == CommandFormat::GuiPreview) {
        const auto& input_files = Get<std::vector<std::string>>("input-files");
        if (!input_files.empty()) {
            const auto& input_desc = m_descriptors.at("input-files");
            // These formats always use long names for clarity.
            command_ss << " " << "--" + input_desc.long_name;

            for (const auto& file : input_files) {
                // These formats always use the full path for the files.
                command_ss << " \"" << file << "\"";
            }
        }
    }

    return command_ss.str();
}

template<typename T>
T ArgumentManager::Get(const std::string& long_name) const {
    if (m_values.count(long_name)) {
        try {
            return std::any_cast<T>(m_values.at(long_name));
        } catch (const std::bad_any_cast& e) {
            throw std::runtime_error("Invalid type requested for argument: " + long_name);
        }
    }
    throw std::runtime_error("Argument not found: " + long_name);
}

// Explicit template instantiation for all types used in the application.
template int ArgumentManager::Get<int>(const std::string& long_name) const;
template double ArgumentManager::Get<double>(const std::string& long_name) const;
template std::string ArgumentManager::Get<std::string>(const std::string& long_name) const;
template std::vector<std::string> ArgumentManager::Get<std::vector<std::string>>(const std::string& long_name) const;
template bool ArgumentManager::Get<bool>(const std::string& long_name) const;