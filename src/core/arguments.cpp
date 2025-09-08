// Fichero: core/Arguments.cpp
#include "Arguments.hpp"
#include "Analysis.hpp"
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

    auto dark_group = app.add_option_group("--black-level", _("Options for the black level"));
    dark_group->add_option("-b,--black-file", opts.dark_file_path, _("Totally dark RAW file (ideally shot at base ISO)"))->check(CLI::ExistingFile);
    dark_group->add_option("--black-level", opts.dark_value, _("Camera RAW black level"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    dark_group->require_option(1); 

    auto sat_group = app.add_option_group("--saturation-level", _("Options for the saturation point"));
    sat_group->add_option("-s,--saturation-file", opts.sat_file_path, _("Totally clipped RAW file (ideally shot at base ISO)"))->check(CLI::ExistingFile);
    sat_group->add_option("--saturation-level", opts.saturation_value, _("Camera RAW saturation level"))->check(CLI::Range(0.0, std::numeric_limits<double>::max()));
    sat_group->require_option(1);
    
    app.add_option("--snrthreshold-db", opts.snr_threshold_db, _("SNR threshold in dB for DR calculation (default=12dB, Photographic DR)"))->default_val(12.0);
    
    // Usa la constante para el valor por defecto del argumento.
    // El valor de opts.poly_order ya se inicializa con DEFAULT_POLY_ORDER en el .hpp,
    // y aquí se sobreescribirá si el usuario pasa el argumento -f.
    app.add_option("--poly-fit,-f", opts.poly_order, _("Polynomic order (default=3) to fit the SNR curve"))->check(CLI::IsMember({2, 3, 4, 5}))->default_val(DEFAULT_POLY_ORDER);
    
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

    return opts;
}