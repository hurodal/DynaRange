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
#include <iomanip>
#include <libintl.h>

#define _(string) gettext(string)

// File: src/core/engine/Initialization.cpp
bool InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {

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

    log_stream << std::fixed << std::setprecision(2);
    log_stream << "\n" << _("[Final configuration]") << std::endl;
    log_stream << _("Black level: ") << opts.dark_value << std::endl;
    log_stream << _("Saturation point: ") << opts.saturation_value << std::endl;
    
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
        case 2: log_stream << _("Graphics with command CLI") << std::endl; break;
    }    
    log_stream << _("Output file: ") << opts.output_filename << "\n" << std::endl;
    
    // --- SETUP PROCESS ORCHESTRATION ---
    
    // Step 1: Extract metadata from all files.
    auto file_info = ExtractFileInfo(opts.input_files, log_stream);
    if (file_info.empty()) {
        log_stream << _("Error: None of the input files could be processed.") << std::endl;
        return false;
    }
    
    // Step 2: Detect sensor resolution if not provided by the user.
    if (opts.sensor_resolution_mpx == 0.0) {
        opts.sensor_resolution_mpx = DetectSensorResolution(opts.input_files, log_stream);
    }

    // Step 3: Determine the final processing order of the files.
    FileOrderResult order = DetermineFileOrder(file_info, log_stream);
    
    // Step 4: Generate the plot labels based on the sorting outcome.
    std::map<std::string, std::string> labels = GeneratePlotLabels(
        order.sorted_filenames,
        file_info,
        order.was_exif_sort_possible
    );
    
    // Step 5: Update the program options with the setup results.
    opts.input_files = order.sorted_filenames;
    opts.plot_labels = labels;
    
    log_stream << _("Sorting finished. Starting Dynamic Range calculation process...") << std::endl;
    
    // Generate command string using the specific 'Plot' format.
    if (opts.plot_mode == 2) {
        opts.generated_command = ArgumentManager::Instance().GenerateCommand(CommandFormat::Plot);
    }
    return true;
}