/**
 * @file core/engine/Initialization.cpp
 * @brief Implementation of the analysis initialization process.
 */
#include "Initialization.hpp"
#include "../Analysis.hpp"
#include "../Arguments.hpp"
#include <iomanip>

// Prepares the analysis: processes dark/sat frames, prints config, and sorts files.
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
    log_stream << "\n[FINAL CONFIGURATION]\n";
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
    log_stream << "Output file: " << opts.output_filename << "\n\n";

    if (!PrepareAndSortFiles(opts, log_stream)) {
        return false;
    }
    
    // Generate command string using the specific 'Plot' format.
    if (opts.plot_mode == 2) {
        opts.generated_command = GenerateCommandString(opts, CommandFormat::Plot);
    }
    return true;
}