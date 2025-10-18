// File: src/core/engine/initialization/ConfigReporter.cpp
/**
 * @file ConfigReporter.cpp
 * @brief Implements the configuration reporting component.
 */
#include "ConfigReporter.hpp"
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace DynaRange::Engine::Initialization {

void ConfigReporter::PrintPreAnalysisTable(const std::vector<FileInfo>& file_info, std::ostream& log_stream) const
{
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
}

/**
 * @brief Prints a summary of the final configuration that will be used for analysis,
 * including calibration values, channel selections, detected resolutions, and the Bayer pattern.
 * @param opts The final program options.
 * @param bayer_pattern The detected Bayer pattern from the RAW file.
 * @param log_stream The output stream for logging.
 */
void ConfigReporter::PrintFinalConfiguration(const ProgramOptions& opts, const std::string& bayer_pattern, std::ostream& log_stream) const
{
    log_stream << std::fixed << std::setprecision(2);
    log_stream << "\n" << _("[Final configuration]") << std::endl;
    log_stream << _("Black level: ") << opts.dark_value 
               << (opts.black_level_is_default ? _(" (estimated)") : "") << std::endl;
    log_stream << _("Saturation point: ") << opts.saturation_value 
               << (opts.saturation_level_is_default ? _(" (estimated)") : "") << std::endl;

    std::vector<std::string> channels_to_print;
    if (opts.raw_channels.R) channels_to_print.push_back("R");
    if (opts.raw_channels.G1) channels_to_print.push_back("G1");
    if (opts.raw_channels.G2) channels_to_print.push_back("G2");
    if (opts.raw_channels.B) channels_to_print.push_back("B");
    if (opts.raw_channels.avg_mode != AvgMode::None) {
        std::string avg_str = "AVG";
        if (opts.raw_channels.avg_mode == AvgMode::Full) {
            avg_str += " (Full)";
        } else { // AvgMode::Selected
            avg_str += " (";
            std::vector<std::string> selected_for_avg;
            if (opts.raw_channels.R) selected_for_avg.push_back("R");
            if (opts.raw_channels.G1) selected_for_avg.push_back("G1");
            if (opts.raw_channels.G2) selected_for_avg.push_back("G2");
            if (opts.raw_channels.B) selected_for_avg.push_back("B");
            
            for (size_t i = 0; i < selected_for_avg.size(); ++i) {
                avg_str += selected_for_avg[i];
                if (i < selected_for_avg.size() - 1) {
                    avg_str += ",";
                }
            }
            avg_str += ")";
        }
        channels_to_print.push_back(avg_str);
    }

    std::stringstream channels_ss;
    for(size_t i = 0; i < channels_to_print.size(); ++i) {
        channels_ss << channels_to_print[i] << (i < channels_to_print.size() - 1 ? ", " : "");
    }
    
    std::string channel_label = (channels_to_print.size() > 1) ?
        _("Analysis channels: ") : _("Analysis channel: ");
    log_stream << channel_label << channels_ss.str() << std::endl;

    if (!bayer_pattern.empty()) {
        log_stream << _("Bayer pattern detected: ") << bayer_pattern << std::endl;
        //if (bayer_pattern != "RGGB") {
        //    log_stream << _("Detected non-RGGB Bayer pattern. Adjusting channel extraction.") << std::endl;
        //}
    }

    if (opts.sensor_resolution_mpx > 0.0) {
        log_stream << _("Sensor resolution: ") << opts.sensor_resolution_mpx << _(" Mpx") << std::endl;
    }

    if (opts.raw_width > 0 && opts.raw_height > 0) {
        log_stream << _("Detected active area: ") << opts.raw_width << "x" << opts.raw_height << " pixels";
        if (opts.raw_width != opts.full_raw_width || opts.raw_height != opts.full_raw_height) {
            log_stream << " (from " << opts.full_raw_width << "x" << opts.full_raw_height << " full with masked pixels)";
        }
        log_stream << std::endl;
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
            case 1: log_stream << _("Graphics without command CLI") << std::endl; break;
            case 2: log_stream << _("Graphics with short command CLI") << std::endl; break;
            case 3: log_stream << _("Graphics with long command CLI") << std::endl; break;
            default: log_stream << _("Graphics enabled") << std::endl; break;
        }
    }
    
    log_stream << _("Output file: ") << opts.output_filename << "\n" << std::endl;
}
} // namespace DynaRange::Engine::Initialization