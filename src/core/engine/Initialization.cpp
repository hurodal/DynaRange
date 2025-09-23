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

bool InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {

    if (!opts.dark_file_path.empty()) {
        auto dark_val_opt = ProcessDarkFrame(opts.dark_file_path, log_stream);
        if (!dark_val_opt) { log_stream << "Fatal error processing dark frame." << std::endl; return false; }
        opts.dark_value = *dark_val_opt;
    }
    if (!opts.sat_file_path.empty()) {
        auto sat_val_opt = ProcessSaturationFrame(opts.sat_file_path, log_stream);
        if (!sat_val_opt) { log_stream << "Fatal error processing saturation frame." << std::endl; return false; }
        opts.saturation_value = *sat_val_opt;
    }

    log_stream << std::fixed << std::setprecision(2);
    log_stream << "\n[Final configuration]\n";
    log_stream << "Black level: " << opts.dark_value << "\n";
    log_stream << "Saturation point: " << opts.saturation_value << "\n";
    
    log_stream << "SNR threshold(s): ";
    for(size_t i = 0; i < opts.snr_thresholds_db.size(); ++i) {
        log_stream << opts.snr_thresholds_db[i] << (i == opts.snr_thresholds_db.size() - 1 ? "" : ", ");
    }
    log_stream << " dB\n";

    log_stream << "DR normalization: " << opts.dr_normalization_mpx << " Mpx\n";
    log_stream << "Polynomic order: " << opts.poly_order << "\n";
    log_stream << "Patch ratio: " << opts.patch_ratio << "\n";
    log_stream << "Plotting: ";
    switch (opts.plot_mode) {
        case 0: log_stream << "No graphics\n"; break;
        case 1: log_stream << "Graphics without command CLI\n"; break;
        case 2: log_stream << "Graphics with command CLI\n"; break;
    }    
    log_stream << "Output file: " << opts.output_filename << "\n\n";

    // --- SETUP PROCESS ORCHESTRATION ---
    
    // Step 1: Extract metadata from all files.
    auto file_info = ExtractFileInfo(opts.input_files, log_stream);
    if (file_info.empty()) {
        log_stream << "Error: None of the input files could be processed." << std::endl;
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
    
    log_stream << "Sorting finished. Starting Dynamic Range calculation process..." << std::endl;
    
    // Generate command string using the specific 'Plot' format.
    if (opts.plot_mode == 2) {
        opts.generated_command = ArgumentManager::Instance().GenerateCommand(CommandFormat::Plot);
    }
    return true;
}