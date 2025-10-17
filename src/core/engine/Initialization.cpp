// File: src/core/engine/Initialization.cpp
/**
 * @file src/core/engine/Initialization.cpp
 * @brief Implementation of the analysis initialization process orchestration.
 */
#include "Initialization.hpp"
#include "initialization/InputFileFilter.hpp"
#include "initialization/CalibrationHandler.hpp"
#include "initialization/ConfigReporter.hpp"
#include "initialization/FileSorter.hpp"
#include "initialization/PreAnalysisRawSelector.hpp"
#include "../setup/MetadataExtractor.hpp"
#include "../setup/PlotLabelGenerator.hpp"
#include "../setup/SensorResolution.hpp"
#include "../utils/CommandGenerator.hpp"
#include <libintl.h>
#include <utility> // For std::pair and std::move

#define _(string) gettext(string)

InitializationResult InitializeAnalysis(const ProgramOptions& opts, std::ostream& log_stream) {
    
    InitializationResult result;
    ProgramOptions local_opts = opts;
    const DynaRange::Engine::Initialization::InputFileFilter file_filter;
    file_filter.Filter(local_opts, log_stream);

    if (local_opts.input_files.empty()) {
        log_stream << _("Error: No valid input files remain after filtering for calibration files.") << std::endl;
        return result;
    }

    log_stream << _("Pre-analyzing files to extract metadata...") << std::endl;
    auto [file_info, loaded_raw_files] = ExtractFileInfo(local_opts.input_files, log_stream);
    
    if (file_info.empty()) {
        log_stream << _("Error: None of the input files could be processed.") << std::endl;
        return result;
    }

    const DynaRange::Engine::Initialization::CalibrationHandler calib_handler;
    if (!calib_handler.HandleCalibration(local_opts, file_info, log_stream)) {
        return result;
    }
    
    const DynaRange::Engine::Initialization::ConfigReporter reporter;
    reporter.PrintPreAnalysisTable(file_info, log_stream);

    FileOrderResult order = DetermineFileOrder(file_info, log_stream);
    local_opts.input_files = order.sorted_filenames;
    local_opts.plot_labels = GeneratePlotLabels(order.sorted_filenames, file_info, order.was_exif_sort_possible);

    // Reorder the loaded_raw_files vector to match the final sorted order
    std::vector<RawFile> sorted_loaded_files;
    sorted_loaded_files.reserve(order.sorted_filenames.size());
    for (const auto& filename : order.sorted_filenames) {
        auto it = std::find_if(loaded_raw_files.begin(), loaded_raw_files.end(),
            [&](RawFile& file){ return file.GetFilename() == filename; });
        if (it != loaded_raw_files.end()) {
            sorted_loaded_files.push_back(std::move(*it));
        }
    }
    loaded_raw_files = std::move(sorted_loaded_files);

    if (local_opts.sensor_resolution_mpx == 0.0) {
        local_opts.sensor_resolution_mpx = DetectSensorResolution(local_opts.input_files, log_stream);
    }
    
    // Store detected dimensions and Bayer pattern from the first valid file.
    if (!loaded_raw_files.empty()) {
        local_opts.raw_width = loaded_raw_files[0].GetActiveWidth();
        local_opts.raw_height = loaded_raw_files[0].GetActiveHeight();
        local_opts.full_raw_width = loaded_raw_files[0].GetWidth();
        local_opts.full_raw_height = loaded_raw_files[0].GetHeight();
        result.bayer_pattern = loaded_raw_files[0].GetFilterPattern(); // Store Bayer pattern
    }
    
    if (local_opts.plot_command_mode == 2) {
        local_opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotShort);
    } else if (local_opts.plot_command_mode == 3) {
        local_opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotLong);
    }
    
    reporter.PrintFinalConfiguration(local_opts, result.bayer_pattern, log_stream); // Pass pattern to reporter
    
    int source_image_index = DynaRange::Engine::Initialization::SelectPreAnalysisRawIndex(loaded_raw_files, local_opts.saturation_value, log_stream);
    
    result.success = true;
    result.loaded_raw_files = std::move(loaded_raw_files);
    result.sorted_filenames = local_opts.input_files;
    result.plot_labels = local_opts.plot_labels;
    result.sensor_resolution_mpx = local_opts.sensor_resolution_mpx;
    result.generated_command = local_opts.generated_command;
    result.dark_value = local_opts.dark_value;
    result.saturation_value = local_opts.saturation_value;
    result.black_level_is_default = local_opts.black_level_is_default;
    result.saturation_level_is_default = local_opts.saturation_level_is_default;
    result.source_image_index = source_image_index;
    
    return result;
}