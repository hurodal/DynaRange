// dynRange.cpp
// Main file for the executable.
// Orchestrates the program flow: argument parsing, file sorting,
// and calling the processing engine.

#include "core/arguments.hpp"
#include "core/engine.hpp"
#include "core/functions.hpp"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <filesystem>
#include <numeric>
#include <algorithm>

// Headers for Gettext
#include <libintl.h>
#include <locale.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

// Temporary structure to associate each file with its brightness.
struct FileExposureInfo {
    std::string filename;
    double mean_brightness;

    // Explicit constructor
    FileExposureInfo(const std::string& name, double brightness)
        : filename(name), mean_brightness(brightness) {}
};

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

    // --- 2. PRE-ANALYSIS AND SORTING BY EXPOSURE ---
    std::vector<FileExposureInfo> exposure_data;
    std::cout << _("Pre-analyzing files to sort by exposure (using fast sampling)...") << std::endl;
    
    for (const std::string& name : opts.input_files) {
        auto mean_val_opt = estimate_mean_brightness(name, 0.05f);

        if (mean_val_opt) {
            exposure_data.emplace_back(name, *mean_val_opt);
            
            std::cout << "  - " << _("File: ") << fs::path(name).filename().string()
                      << ", " << _("Estimated brightness: ") << std::fixed << std::setprecision(2) << *mean_val_opt << std::endl;
        }
    }
    
    // Sort the list of files based on mean brightness.
    std::sort(exposure_data.begin(), exposure_data.end(),
        [](const FileExposureInfo& a, const FileExposureInfo& b) {
            return a.mean_brightness < b.mean_brightness;
        }
    );

    // Update the 'opts' file list with the now-sorted list.
    opts.input_files.clear();
    for (const auto& info : exposure_data) {
        opts.input_files.push_back(info.filename);
    }
    
    std::cout << _("Sorting finished. Starting Dynamic Range calculation process...") << std::endl;
    
    // --- 3. CALL THE PROCESSING ENGINE ---
    // Pass the complete configuration and the console (std::cout) as the output stream for logs.
    if (!run_dynamic_range_analysis(opts, std::cout)) {
        std::cerr << _("A critical error occurred during processing. Please check the log.") << std::endl;
        return 1;
    }
    
    return 0;
}