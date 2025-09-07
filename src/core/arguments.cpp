// Fichero: core/Arguments.cpp
#include "Arguments.hpp"
#include "Analysis.hpp" // Depende de las funciones de análisis
#include <CLI/CLI.hpp>
#include <iostream>
#include <limits>
#include <clocale>
#include <libintl.h>

#define _(string) gettext(string)

ProgramOptions ParseArguments(int argc, char* argv[]) {
    char* current_locale = setlocale(LC_NUMERIC, nullptr);
    setlocale(LC_NUMERIC, "C");

    ProgramOptions opts{};
    CLI::App app{_("Calculates the dynamic range from a series of RAW images.")};

    std::string dark_file, sat_file;
    double dark_value_cli = -1.0, sat_value_cli = -1.0;

    auto dark_group = app.add_option_group("--black-level", _("Options for the black level"));
    auto opt_dark_file = dark_group->add_option("-b,--black-file", dark_file, _("Totally dark RAW file (ideally shot at base ISO)"))->check(CLI::ExistingFile);
    auto opt_dark_value = dark_group->add_option("--black-level", dark_value_cli, _("Camera RAW black level"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    dark_group->require_option(1); 

    auto sat_group = app.add_option_group("--saturation-level", _("Options for the saturation point"));
    auto opt_sat_file = sat_group->add_option("-s,--saturation-file", sat_file, _("Totally clipped RAW file (ideally shot at base ISO)"))->check(CLI::ExistingFile);
    auto opt_sat_value = sat_group->add_option("--saturation-level", sat_value_cli, _("Camera RAW saturation level"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    sat_group->require_option(1);
    
    // Nuevos argumentos, se han añadido con la misma sintaxis y descripciones que en la captura de pantalla
    app.add_option("--chart", opts.chart_colors, _("Create a test chart in PNG format ranging colours from (0,0,0) to (R,G,B) with gamma"))->group(_("Test Chart"));
    app.add_option("--snrthreshold-db", opts.snr_threshold_db, _("SNR threshold in dB for DR calculation (default=12dB, Photographic DR)"))->default_val(12.0);
    app.add_option("--poly-fit,-f", opts.poly_order, _("Polynomic order (default=3) to fit the SNR curve"))->check(CLI::IsMember({2, 3, 4, 5}))->default_val(3);
    app.add_option("--drnormalization-mpx,-m", opts.dr_normalization_mpx, _("Number of Mpx for DR normalization (default=8Mpx)"))->default_val(8.0);
    app.add_option("--patch-safe,-p", opts.patch_safe, _("Number of border safety pixels around each patch (default=50px)"))->default_val(50);
    app.add_option("-i,--input-files", opts.input_files, _("Input RAW files shot over the magenta test chart (ideally for every ISO)"))->required()->check(CLI::ExistingFile);
    app.add_option("-o,--output-file", opts.output_filename, _("Output filename with all results (black level, sat level, SNR samples, DR values)"))->default_val("DR_results.csv");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        setlocale(LC_NUMERIC, current_locale);
        exit(app.exit(e));
    }

    setlocale(LC_NUMERIC, current_locale);

    if (*opt_dark_file) {
        auto dark_val_opt = ProcessDarkFrame(dark_file, std::cout);
        if (!dark_val_opt) {
            std::cerr << "Fatal error: Could not process dark file: " << dark_file << ". Exiting." << std::endl;
            exit(1);
        }
        opts.dark_value = *dark_val_opt;
    } else {
        opts.dark_value = dark_value_cli;
    }

    if (*opt_sat_file) {
        auto sat_val_opt = ProcessSaturationFrame(sat_file, std::cout);
        if (!sat_val_opt) {
            std::cerr << "Fatal error: Could not process saturation file: " << sat_file << ". Exiting." << std::endl;
            exit(1);
        }
        opts.saturation_value = *sat_val_opt;
    } else {
        opts.saturation_value = sat_value_cli;
    }

    return opts;
}