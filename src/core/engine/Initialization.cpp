// File: src/core/engine/Initialization.cpp
/**
 * @file src/core/engine/Initialization.cpp
 * @brief Implementation of the analysis initialization process.
 */
#include "Initialization.hpp"
#include "../arguments/ArgumentManager.hpp"
#include "../analysis/RawProcessor.hpp"
#include "../setup/MetadataExtractor.hpp"
#include "../setup/SensorResolution.hpp"
#include "../setup/FileSorter.hpp"
#include "../setup/PlotLabelGenerator.hpp"
#include "../utils/CommandGenerator.hpp" 
#include <set> 
#include <iomanip>
#include <libintl.h>

#define _(string) gettext(string)

// File: src/core/engine/Initialization.cpp
/**
 * @file src/core/engine/Initialization.cpp
 * @brief Implementation of the analysis initialization process.
 */
#include "Initialization.hpp"
#include "../arguments/ArgumentManager.hpp"
#include "../analysis/RawProcessor.hpp"
#include "../setup/MetadataExtractor.hpp"
#include "../setup/SensorResolution.hpp"
#include "../setup/FileSorter.hpp"
#include "../setup/PlotLabelGenerator.hpp"
#include "../utils/CommandGenerator.hpp" // MODIFIED: Added include for the new module
#include <set> 
#include <iomanip>
#include <libintl.h>

#define _(string) gettext(string)

bool InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {

    // --- Deduplicate Input Files ---
    if (!opts.input_files.empty()) {
        std::vector<std::string> unique_files;
        std::set<std::string> seen_files;
        unique_files.reserve(opts.input_files.size());
        for (const auto& file : opts.input_files) {
            if (seen_files.insert(file).second) {
                unique_files.push_back(file);
            } else {
                log_stream << _("Warning: Duplicate input file ignored: ") << file << std::endl;
            }
        }
        opts.input_files = unique_files;
    }

    // --- The rest of the function continues below ---
    if (!opts.dark_file_path.empty()) {
        auto dark_val_opt = ProcessDarkFrame(opts.dark_file_path, log_stream);
        if (!dark_val_opt) { log_stream << _("Fatal error processing dark frame.") << std::endl; return false; }
        opts.dark_value = *dark_val_opt;
    }
    if (!opts.sat_file_path.empty()) {
        auto sat_val_opt = ProcessSaturationFrame(opts.sat_file_path, log_stream);
        if (!sat_val_opt) { log_stream << _("Fatal error processing saturation frame.") << std::endl; return false; }
        opts.saturation_value = *sat_val_opt;
    }

    // --- SETUP PROCESS ORCHESTRATION ---
    auto file_info = ExtractFileInfo(opts.input_files, log_stream);
    if (file_info.empty()) {
        log_stream << _("Error: None of the input files could be processed.") << std::endl;
        return false;
    }
    
    if (opts.sensor_resolution_mpx == 0.0) {
        opts.sensor_resolution_mpx = DetectSensorResolution(opts.input_files, log_stream);
    }

    FileOrderResult order = DetermineFileOrder(file_info, log_stream);
    std::map<std::string, std::string> labels = GeneratePlotLabels(
        order.sorted_filenames,
        file_info,
        order.was_exif_sort_possible
    );
    opts.input_files = order.sorted_filenames;
    opts.plot_labels = labels;

    // --- PRINT FINAL CONFIGURATION ---
    log_stream << std::fixed << std::setprecision(2);
    log_stream << "\n" << _("[Final configuration]") << std::endl;
    log_stream << _("Black level: ") << opts.dark_value << std::endl;
    log_stream << _("Saturation point: ") << opts.saturation_value << std::endl;
    if (opts.sensor_resolution_mpx > 0.0) {
        log_stream << _("Sensor resolution: ") << opts.sensor_resolution_mpx << _(" Mpx") << std::endl;
    }
    log_stream << _("SNR threshold(s): ");
    for(size_t i = 0; i < opts.snr_thresholds_db.size(); ++i) {
        log_stream << opts.snr_thresholds_db[i] << (i == opts.snr_thresholds_db.size() - 1 ? "" : ", ");
    }
    log_stream << _(" dB") << std::endl;
    log_stream << _("DR normalization: ") << opts.dr_normalization_mpx << _(" Mpx") << std::endl;
    log_stream << _("Polynomic order: ") << opts.poly_order << std::endl;
    log_stream << _("Patch ratio: ") << opts.patch_ratio << std::endl;
    log_stream << _("Plotting: ");
    switch (opts.plot_mode) {
        case 0: log_stream << _("No graphics") << std::endl; break;
        case 1: log_stream << _("Graphics without command CLI") << std::endl; break;
        case 2: log_stream << _("Graphics with short command CLI") << std::endl; break;
        case 3: log_stream << _("Graphics with long command CLI") << std::endl; break;
    }    
    log_stream << _("Output file: ") << opts.output_filename << "\n" << std::endl;
    log_stream << _("Starting Dynamic Range calculation process...") << std::endl;
    
    // Generate command string based on the selected plot mode.
    // The call now uses the new CommandGenerator module.
    if (opts.plot_mode == 2) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotShort);
    } else if (opts.plot_mode == 3) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotLong);
    }
    
    return true;
}