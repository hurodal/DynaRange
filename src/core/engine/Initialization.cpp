// File: src/core/engine/Initialization.cpp
/**
 * @file src/core/engine/Initialization.cpp
 * @brief Implementation of the analysis initialization process.
 */
#include "Initialization.hpp"
#include "../analysis/RawProcessor.hpp"
#include "../setup/MetadataExtractor.hpp"
#include "../setup/SensorResolution.hpp"
#include "../setup/FileSorter.hpp"
#include "../setup/PlotLabelGenerator.hpp"
#include "../utils/CommandGenerator.hpp"
#include "../Constants.hpp"
#include <set> 
#include <iomanip>
#include <filesystem>
#include <libintl.h>
#include <cstring>

#define _(string) gettext(string)

namespace fs = std::filesystem;

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

    if (!opts.dark_file_path.empty()) {
        auto dark_val_opt = ProcessDarkFrame(opts.dark_file_path, log_stream);
        if (!dark_val_opt) { log_stream << _("Fatal error processing dark frame.") << std::endl; return false;
        }
        opts.dark_value = *dark_val_opt;
    }
    if (!opts.sat_file_path.empty()) {
        auto sat_val_opt = ProcessSaturationFrame(opts.sat_file_path, log_stream);
        if (!sat_val_opt) { log_stream << _("Fatal error processing saturation frame.") << std::endl; return false;
        }
        opts.saturation_value = *sat_val_opt;
    }

    // --- SETUP PROCESS ORCHESTRATION ---
    log_stream << _("Pre-analyzing files to determine sorting order...") << std::endl;
    auto file_info = ExtractFileInfo(opts.input_files, log_stream);
    if (file_info.empty()) {
        log_stream << _("Error: None of the input files could be processed.") << std::endl;
        return false;
    }

    // --- Impresión de tabla con anchos dinámicos ---
    // 1. Medir anchos máximos
    size_t max_file_width = strlen("File");
    size_t max_bright_width = strlen("Brightness");
    size_t max_iso_width = strlen("ISO");

    for (const auto& info : file_info) {
        max_file_width = std::max(max_file_width, fs::path(info.filename).filename().string().length());
        std::stringstream bright_ss;
        bright_ss << std::fixed << std::setprecision(2) << info.mean_brightness;
        max_bright_width = std::max(max_bright_width, bright_ss.str().length());

        std::stringstream iso_ss;
        iso_ss << static_cast<int>(info.iso_speed);
        max_iso_width = std::max(max_iso_width, iso_ss.str().length());
    }

    // Añadir un padding de 2 espacios
    max_file_width += 2;
    max_bright_width += 2;
    max_iso_width += 2;

    // 2. Imprimir la tabla
    log_stream << "  " << std::left << std::setw(max_file_width) << "File"
               << std::right << std::setw(max_bright_width) << "Brightness"
               << std::right << std::setw(max_iso_width) << "ISO" << std::endl;
    log_stream << "  " << std::string(max_file_width + max_bright_width + max_iso_width, '-') << std::endl;
    for (const auto& info : file_info) {
        log_stream << "  " << std::left << std::setw(max_file_width) << fs::path(info.filename).filename().string()
                   << std::right << std::setw(max_bright_width) << std::fixed << std::setprecision(2) << info.mean_brightness
                   << std::right << std::setw(max_iso_width) << static_cast<int>(info.iso_speed) << std::endl;
    }
    
    FileOrderResult order = DetermineFileOrder(file_info, log_stream);
    std::map<std::string, std::string> labels = GeneratePlotLabels(
        order.sorted_filenames,
        file_info,
        order.was_exif_sort_possible
    );
    opts.input_files = order.sorted_filenames;
    opts.plot_labels = labels;

    // Detect sensor resolution after sorting messages have been printed.
    if (opts.sensor_resolution_mpx == 0.0) {
        opts.sensor_resolution_mpx = DetectSensorResolution(opts.input_files, log_stream);
    }

    // --- PRINT FINAL CONFIGURATION ---
    log_stream << std::fixed << std::setprecision(2);
    log_stream << "\n" << _("[Final configuration]") << std::endl;
    log_stream << _("Black level: ") << opts.dark_value << std::endl;
    log_stream << _("Saturation point: ") << opts.saturation_value << std::endl;

    // Add a log message to indicate which Bayer channel is being used.
    std::string channel_name;
    switch (DynaRange::Constants::BAYER_CHANNEL_TO_ANALYZE) {
        case DynaRange::Constants::BayerChannel::R:
            channel_name = "Red (R)";
            break;
        case DynaRange::Constants::BayerChannel::G1:
            channel_name = "Green 1 (G1)";
            break;
        case DynaRange::Constants::BayerChannel::G2:
            channel_name = "Green 2 (G2)";
            break;
        case DynaRange::Constants::BayerChannel::B:
            channel_name = "Blue (B)";
            break;
        default:
            channel_name = "Unknown";
            break;
    }
    log_stream << _("Analysis channel: ") << channel_name << std::endl;

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
        case 0: log_stream << _("No graphics") << std::endl;
        break;
        case 1: log_stream << _("Graphics without command CLI") << std::endl; break;
        case 2: log_stream << _("Graphics with short command CLI") << std::endl; break;
        case 3: log_stream << _("Graphics with long command CLI") << std::endl; break;
    }    
    log_stream << _("Output file: ") << opts.output_filename << "\n" << std::endl;
    
    if (opts.plot_mode == 2) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotShort);
    } else if (opts.plot_mode == 3) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotLong);
    }
    
    return true;
}