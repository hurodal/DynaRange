// dynRange.cpp
// Main file for the CLI executable.
// Orchestrates the program flow: argument parsing, file sorting,
// and calling the processing engine.

#include "core/Analysis.hpp"
#include "core/Arguments.hpp"
#include "core/Engine.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

// Headers for Gettext
#include <libintl.h>
#include <locale.h>

#define _(string) gettext(string)

// --- MAIN FUNCTION ---
int main(int argc, char* argv[]) {

    // --- GETTEXT INITIALIZATION ---
    setlocale(LC_ALL, "");
    bindtextdomain("dynrange", "locale");
    textdomain("dynrange");

    // --- 1. ARGUMENT PARSING ---
    ProgramOptions opts = ParseArguments(argc, argv);
    
    // Display the final configuration.
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n" << _("[FINAL CONFIGURATION]") << "\n";
    std::cout << _("Black level: ") << opts.dark_value << "\n";
    std::cout << _("Saturation point: ") << opts.saturation_value << "\n";
    // AÑADIDO: Muestra los nuevos argumentos de configuración
    std::cout << _("SNR threshold: ") << opts.snr_threshold_db << " dB\n";
    std::cout << _("DR normalization: ") << opts.dr_normalization_mpx << " Mpx\n";
    std::cout << _("Polynomic order: ") << opts.poly_order << "\n";
    std::cout << _("Patch safe: ") << opts.patch_safe << " px\n";
    
    std::cout << _("Output file: ") << opts.output_filename << "\n\n";

    // --- 2. PREPARE AND SORT FILES ---
    // This centralized function handles the pre-analysis and sorting logic.
    if (!PrepareAndSortFiles(opts, std::cout)) {
        return 1; // Exit if preparation fails
    }
    
    // --- 3. CALL THE PROCESSING ENGINE ---
    // Pass the complete configuration and the console (std::cout) as the output stream for logs.
    if (!RunDynamicRangeAnalysis(opts, std::cout)) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    
    return 0;
}