// File: src/core/engine/Initialization.cpp
/**
 * @file src/core/engine/Initialization.cpp
 * @brief Implementation of the analysis initialization process.
 */
#include "Initialization.hpp"
#include "../analysis/RawProcessor.hpp"
#include "../setup/CalibrationEstimator.hpp"
#include "../setup/FileSorter.hpp"
#include "../setup/MetadataExtractor.hpp"
#include "../setup/PlotLabelGenerator.hpp"
#include "../setup/SensorResolution.hpp"
#include "../utils/CommandGenerator.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <libintl.h>
#include <set> 

#define _(string) gettext(string)

namespace fs = std::filesystem;

bool InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {

    // --- 1. Exclude Calibration Files from Analysis ---
    if (!opts.dark_file_path.empty() || !opts.sat_file_path.empty()) {
        std::set<std::string> calibration_files;
        if (!opts.dark_file_path.empty()) {
            calibration_files.insert(opts.dark_file_path);
        }
        if (!opts.sat_file_path.empty()) {
            calibration_files.insert(opts.sat_file_path);
        }

        std::vector<std::string> files_to_remove;
        auto original_size = opts.input_files.size();

        opts.input_files.erase(
            std::remove_if(opts.input_files.begin(), opts.input_files.end(),
                [&](const std::string& input_file) {
                    if (calibration_files.count(input_file)) {
                        files_to_remove.push_back(input_file);
                        return true;
                    }
                    return false;
                }),
            opts.input_files.end()
        );

        if (!files_to_remove.empty()) {
            log_stream << _("[INFO] The following files were excluded from the analysis because they are used for calibration:") << std::endl;
            for (const auto& file : files_to_remove) {
                log_stream << "  - " << fs::path(file).filename().string() << std::endl;
            }
        }
    }

    // --- 2. Deduplicate Input Files ---
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

    // --- 3. Pre-analysis of all files to get metadata (moved to the beginning) ---
    log_stream << _("Pre-analyzing files to extract metadata...") << std::endl;
    auto file_info = ExtractFileInfo(opts.input_files, log_stream);
    if (file_info.empty()) {
        log_stream << _("Error: None of the input files could be processed.") << std::endl;
        return false;
    }

    // --- 4. DEFAULT CALIBRATION ESTIMATION (now uses pre-analyzed data) ---
    if (opts.dark_file_path.empty() && opts.black_level_is_default) {
        log_stream << _("[INFO] Black level not specified. Attempting to estimate from RAW file...") << std::endl;
        auto estimated_black = CalibrationEstimator::EstimateBlackLevel(opts, file_info, log_stream);
        if (estimated_black) {
            opts.dark_value = *estimated_black;
        } else {
            log_stream << _("[Warning] Could not estimate black level. Using fallback default value: ") 
                       << opts.dark_value << std::endl;
        }
    }

    if (opts.sat_file_path.empty() && opts.saturation_level_is_default) {
        log_stream << _("[INFO] Saturation level not specified. Attempting to estimate from RAW file...") << std::endl;
        auto estimated_sat = CalibrationEstimator::EstimateSaturationLevel(opts, file_info, log_stream);
        if (estimated_sat) {
            opts.saturation_value = *estimated_sat;
        } else {
            log_stream << _("[Warning] Could not estimate saturation level. Using fallback default value: ")
                       << opts.saturation_value << std::endl;
        }
    }

    // --- 5. CALIBRATION FROM EXPLICIT FILES ---
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

    // --- 6. SETUP PROCESS ORCHESTRATION (using already extracted metadata) ---
    // --- Print table with dynamic widths ---
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

    max_file_width += 2;
    max_bright_width += 2;
    max_iso_width += 2;
    log_stream << "\n" << _("Sorting files based on pre-analyzed data:") << std::endl;
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

    if (opts.sensor_resolution_mpx == 0.0) {
        opts.sensor_resolution_mpx = DetectSensorResolution(opts.input_files, log_stream);
    }

    // --- 7. PRINT FINAL CONFIGURATION ---
    log_stream << std::fixed << std::setprecision(2);
    log_stream << "\n" << _("[Final configuration]") << std::endl;
    log_stream << _("Black level: ") << opts.dark_value 
               << (opts.black_level_is_default ? _(" (estimated)") : "") << std::endl;
    log_stream << _("Saturation point: ") << opts.saturation_value 
               << (opts.saturation_level_is_default ? _(" (estimated)") : "") << std::endl;
    // New logic to dynamically build the list of channels to be analyzed.
    std::vector<std::string> selected_channels;
    if (opts.raw_channels.R) selected_channels.push_back("R");
    if (opts.raw_channels.G1) selected_channels.push_back("G1");
    if (opts.raw_channels.G2) selected_channels.push_back("G2");
    if (opts.raw_channels.B) selected_channels.push_back("B");
    if (opts.raw_channels.AVG) selected_channels.push_back("AVG");

    std::stringstream channels_ss;
    for(size_t i = 0; i < selected_channels.size(); ++i) {
        channels_ss << selected_channels[i] << (i < selected_channels.size() - 1 ? ", " : "");
    }
    
    std::string channel_label = (selected_channels.size() > 1) ?
_("Analysis channels: ") : _("Analysis channel: ");
    log_stream << channel_label << channels_ss.str() << std::endl;
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
    if (!opts.generate_plot) {
        log_stream << _("No graphics") << std::endl;
    } else {
        switch (opts.plot_command_mode) {
            case 1: log_stream << _("Graphics without command CLI") << std::endl;
            break;
            case 2: log_stream << _("Graphics with short command CLI") << std::endl; break;
            case 3: log_stream << _("Graphics with long command CLI") << std::endl; break;
            default: log_stream << _("Graphics enabled") << std::endl; break;
        }
    }
    
    log_stream << _("Output file: ") << opts.output_filename << "\n" << std::endl;
    if (opts.plot_command_mode == 2) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotShort);
    } else if (opts.plot_command_mode == 3) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotLong);
    }
    
    return true;
}