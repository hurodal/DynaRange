// File: src/core/arguments/CommandLineParser.cpp
/**
 * @file src/core/arguments/CommandLineParser.cpp
 * @brief Implements the command-line argument parser using CLI++.
 * @details This is the sole implementation of the parsing logic. It depends
 *          on CLI11 and the ProgramOptions definition. It has no knowledge
 *          of how commands are generated.
 */
#include "CommandLineParser.hpp"
#include <CLI/CLI.hpp>
#include <limits>
#include <clocale>
#include <libintl.h>

#define _(string) gettext(string)

#ifdef _WIN32
#include <windows.h>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Función para expandir un único patrón de fichero en Windows.
void expand_single_wildcard(const std::string& pattern, std::vector<std::string>& expanded_files) {
    WIN32_FIND_DATAA find_data;
    HANDLE h_find = FindFirstFileA(pattern.c_str(), &find_data);

    if (h_find != INVALID_HANDLE_VALUE) {
        fs::path pattern_path(pattern);
        fs::path parent_dir = pattern_path.parent_path();
        do {
            if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                expanded_files.push_back((parent_dir / find_data.cFileName).string());
            }
        } while (FindNextFileA(h_find, &find_data) != 0);
        FindClose(h_find);
    }
}

// Función principal que procesa la lista de argumentos y expande los que contengan wildcards.
std::vector<std::string> expand_wildcards_on_windows(const std::vector<std::string>& files) {
    std::vector<std::string> result_files;
    for (const auto& file_arg : files) {
        // Un argumento es un patrón si contiene '*' o '?'.
        if (file_arg.find_first_of("*?") != std::string::npos) {
            expand_single_wildcard(file_arg, result_files);
        } else {
            // Si no tiene wildcards, se añade directamente.
            result_files.push_back(file_arg);
        }
    }
    return result_files;
}

#endif // _WIN32

ProgramOptions ParseCommandLine(int argc, char* argv[]) {
    char* current_locale = setlocale(LC_NUMERIC, nullptr);
    setlocale(LC_NUMERIC, "C");
    ProgramOptions opts{};
    CLI::App app{_("Calculates the dynamic range from a series of RAW images.")};
    app.allow_extras(); // Allow positional file arguments

    // --- Chart creation mode (unimplemented functionality) ---
    std::vector<double> chart_params;
    auto chart_opt = app.add_option("-c,--chart", chart_params, _("Create a test chart in PNG format ranging colours from (0,0,0) to (R,G,B) with gamma"))->expected(4);

    // --- Main analysis options ---
    app.add_option("-B,--black-file", opts.dark_file_path, _("Totally dark RAW file (ideally shot at base ISO)"))->check(CLI::ExistingFile);
    app.add_option("-b,--black-level", opts.dark_value, _("Camera RAW black level"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    app.add_option("-S,--saturation-file", opts.sat_file_path, _("Totally clipped RAW file (ideally shot at base ISO)"))->check(CLI::ExistingFile);
    app.add_option("-s,--saturation-level", opts.saturation_value, _("Camera RAW saturation level"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    app.add_option("-i,--input-files", opts.input_files, _("Input RAW files shot over the magenta test chart (ideally for every ISO)"))->required();
    app.add_option("-o,--output-file", opts.output_filename, _("Output filename with all results (black level, sat level, SNR samples, DR values)"))->default_val("DR_results.csv");

    // --- Calculation parameters ---
    double snr_val;
    auto snr_opt = app.add_option("-d,--snrthreshold-db", snr_val, _("SNR threshold in dB for DR calculation (default=12dB and 0dB)"));
    app.add_option("-m,--drnormalization-mpx", opts.dr_normalization_mpx, _("Number of Mpx for DR normalization (default=8Mpx)"))->default_val(8.0);
    //app.add_option("--sensor-resolution-mpx", opts.sensor_resolution_mpx, _("Sensor resolution in megapixels (if unknown, set to 0 to auto-detect)"))->default_val(0.0);
    app.add_option("-f,--poly-fit", opts.poly_order, _("Polynomic order (default=3) to fit the SNR curve"))->check(CLI::Range(2, 3))->default_val(DEFAULT_POLY_ORDER);
    app.add_option("-r,--patch-ratio", opts.patch_ratio, _("Relative patch width/height used to compute signal and noise readings"))->check(CLI::Range(0.0, 1.0))->default_val(0.5);
    app.add_option("-p,--plot", opts.plot_mode, _("Export SNR curves in PNG format (0=no, 1=plot, 2=plot+command)"))->check(CLI::Range(0, 2))->default_val(0);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        setlocale(LC_NUMERIC, current_locale);
        exit(app.exit(e));
    }

    // Expansión de wildcards específicamente para Windows.
    #ifdef _WIN32
    if (!opts.input_files.empty()) {
        opts.input_files = expand_wildcards_on_windows(opts.input_files);
    }
    #endif

    // --- Post-parsing logic ---
    if (chart_opt->count() > 0) {
        opts.create_chart_mode = true;
        opts.chart_params = chart_params;
    }
    if (snr_opt->count() > 0) {
        opts.snr_thresholds_db.push_back(snr_val);
    } else {
        opts.snr_thresholds_db = {12.0, 0.0}; // Default behavior
    }

    setlocale(LC_NUMERIC, current_locale);
    return opts;
}