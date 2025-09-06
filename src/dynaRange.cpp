// dynRange.cpp
// Main file for the CLI executable.
// Orchestrates the program flow: argument parsing, file sorting,
// and calling the processing engine.

#include "core/arguments.hpp"
#include "core/engine.hpp"
#include "core/functions.hpp"

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
    ProgramOptions opts = parse_arguments(argc, argv);
    
    // Display the final configuration.
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n" << _("[FINAL CONFIGURATION]") << "\n";
    std::cout << _("Black level: ") << opts.dark_value << "\n";
    std::cout << _("Saturation point: ") << opts.saturation_value << "\n";
    std::cout << _("Output file: ") << opts.output_filename << "\n\n";

    // --- 2. PREPARE AND SORT FILES ---
    // This centralized function handles the pre-analysis and sorting logic.
    if (!prepare_and_sort_files(opts, std::cout)) {
        return 1; // Exit if preparation fails
    }
    
    // --- 3. CALL THE PROCESSING ENGINE ---
    // Pass the complete configuration and the console (std::cout) as the output stream for logs.
    if (!run_dynamic_range_analysis(opts, std::cout)) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    
    return 0;
}