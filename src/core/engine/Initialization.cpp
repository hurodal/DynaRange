// Fichero: core/engine/Initialization.cpp
#include "Initialization.hpp"
#include "../Analysis.hpp"
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

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
    log_stream << "SNR threshold: " << opts.snr_threshold_db << " dB\n";
    log_stream << "DR normalization: " << opts.dr_normalization_mpx << " Mpx\n";
    log_stream << "Polynomic order: " << opts.poly_order << "\n";
    log_stream << "Patch safe: " << opts.patch_safe << " px\n";
    log_stream << "Output file: " << opts.output_filename << "\n\n";

    if (!PrepareAndSortFiles(opts, log_stream)) {
        return false;
    }
    
    if (opts.report_command) {
        std::stringstream command_ss;
        command_ss << std::fixed << std::setprecision(2);
        command_ss << "dynaRange --report-command";
        if (!opts.dark_file_path.empty()) {
            command_ss << " --black-file \"" << fs::path(opts.dark_file_path).filename().string() << "\"";
        } else {
            command_ss << " --black-level " << opts.dark_value;
        }
        if (!opts.sat_file_path.empty()) {
            command_ss << " --saturation-file \"" << fs::path(opts.sat_file_path).filename().string() << "\"";
        } else {
            command_ss << " --saturation-level " << opts.saturation_value;
        }
        command_ss << " --poly-fit " << opts.poly_order;
        command_ss << " --patch-safe " << opts.patch_safe;
        command_ss << " --snrthreshold-db " << opts.snr_threshold_db;
        command_ss << " --drnormalization-mpx " << opts.dr_normalization_mpx;
        opts.generated_command = command_ss.str();
    }
    return true;
}