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
#include "../setup/MetadataExtractor.hpp"
#include "../setup/PlotLabelGenerator.hpp"
#include "../setup/SensorResolution.hpp"
#include "../utils/CommandGenerator.hpp"
#include "../io/raw/RawFile.hpp"
#include <libintl.h>
#include <utility> // For std::pair and std::move

#define _(string) gettext(string)

std::pair<bool, std::vector<RawFile>> InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream) {
    
    const DynaRange::Engine::Initialization::InputFileFilter file_filter;
    file_filter.Filter(opts, log_stream);

    if (opts.input_files.empty()) {
        log_stream << _("Error: No valid input files remain after filtering for calibration files.") << std::endl;
        return {false, std::vector<RawFile>{}};
    }

    log_stream << _("Pre-analyzing files to extract metadata...") << std::endl;
    auto [file_info, loaded_raw_files] = ExtractFileInfo(opts.input_files, log_stream);
    
    if (file_info.empty()) {
        log_stream << _("Error: None of the input files could be processed.") << std::endl;
        return {false, std::vector<RawFile>{}};
    }

    const DynaRange::Engine::Initialization::CalibrationHandler calib_handler;
    if (!calib_handler.HandleCalibration(opts, file_info, log_stream)) {
        return {false, std::vector<RawFile>{}};
        // Fatal error during calibration file processing
    }
    
    const DynaRange::Engine::Initialization::ConfigReporter reporter;
    reporter.PrintPreAnalysisTable(file_info, log_stream);

    FileOrderResult order = DetermineFileOrder(file_info, log_stream);
    opts.input_files = order.sorted_filenames;
    opts.plot_labels = GeneratePlotLabels(order.sorted_filenames, file_info, order.was_exif_sort_possible);
    
    if (opts.sensor_resolution_mpx == 0.0) {
        opts.sensor_resolution_mpx = DetectSensorResolution(opts.input_files, log_stream);
    }
    
    reporter.PrintFinalConfiguration(opts, log_stream);
    
    if (opts.plot_command_mode == 2) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotShort);
    } else if (opts.plot_command_mode == 3) {
        opts.generated_command = CommandGenerator::GenerateCommand(CommandFormat::PlotLong);
    }
    
    return {true, std::move(loaded_raw_files)};
}