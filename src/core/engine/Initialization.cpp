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
#include "../setup/PreAnalysis.hpp" // <<-- Necesario para PreAnalysisResult
#include "../setup/Constants.hpp"
#include <libintl.h>
#include <opencv2/core.hpp>
#include <utility> // For std::pair and std::move
#include <map>     // <<-- Necesario para std::map
#include <algorithm> // <<-- Necesario para std::find_if

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
    // ExtractFileInfo ahora devuelve FileInfo Y RawFile cargados
    auto [initial_file_info_vec, loaded_raw_files] = ExtractFileInfo(local_opts.input_files, log_stream);

    if (initial_file_info_vec.empty()) {
        log_stream << _("Error: None of the input files could be processed.") << std::endl;
        return result;
    }

    // --- Obtener los resultados del pre-análisis (que incluyen has_saturated_pixels) ---
    // ExtractFileInfo ya hizo la llamada a PreAnalyzeRawFiles internamente.
    // Necesitamos reconstruir esa información aquí usando los FileInfo y RawFile que tenemos.
    // Esto es un poco ineficiente, idealmente ExtractFileInfo devolvería directamente PreAnalysisResult.
    std::vector<PreAnalysisResult> pre_analysis_results;
    pre_analysis_results.reserve(initial_file_info_vec.size());
    for(const auto& finfo : initial_file_info_vec) {
        // Encontrar el RawFile correspondiente para la comprobación de saturación
        auto raw_it = std::find_if(loaded_raw_files.begin(), loaded_raw_files.end(),
            [&](const RawFile& rf){ return rf.GetFilename() == finfo.filename; });

        bool is_saturated = false; // Asumir no saturado si falla algo
        if (raw_it != loaded_raw_files.end()) {
             cv::Mat active_img = raw_it->GetActiveRawImage();
             if (!active_img.empty()) {
                 // Usar local_opts.saturation_value aquí asume que ya ha sido calculado
                 // por CalibrationHandler. Esto podría ser problemático si no.
                 // Sería más seguro usar un valor temporal alto aquí como hacía ExtractFileInfo
                 // y que SelectPreAnalysisRawIndex reciba el valor final después de CalibrationHandler.
                 // Por ahora, asumimos que HandleCalibration se llama antes.
                 int saturated_pixels = cv::countNonZero(active_img >= (local_opts.saturation_value * 0.99));
                 double total_pixels = active_img.total();
                 double saturation_ratio = (total_pixels > 0) ? static_cast<double>(saturated_pixels) / total_pixels : 0.0;
                 is_saturated = (saturation_ratio > DynaRange::Setup::Constants::MAX_PRE_ANALYSIS_SATURATION_RATIO);
             }
        }
        pre_analysis_results.push_back({finfo.filename, finfo.mean_brightness, finfo.iso_speed, is_saturated, local_opts.saturation_value});
    }
    // ---------------------------------------------------------------------------------


    const DynaRange::Engine::Initialization::CalibrationHandler calib_handler;
    // ¡Importante! HandleCalibration puede cambiar local_opts.saturation_value
    if (!calib_handler.HandleCalibration(local_opts, initial_file_info_vec, log_stream)) {
        return result;
    }

    // --- RE-CALCULAR has_saturated_pixels con el valor de saturación FINAL ---
    // (Esto es necesario si HandleCalibration cambió saturation_value)
    for(auto& pa_result : pre_analysis_results) {
         auto raw_it = std::find_if(loaded_raw_files.begin(), loaded_raw_files.end(),
            [&](const RawFile& rf){ return rf.GetFilename() == pa_result.filename; });
         if (raw_it != loaded_raw_files.end()) {
             cv::Mat active_img = raw_it->GetActiveRawImage();
             if (!active_img.empty()) {
                 int saturated_pixels = cv::countNonZero(active_img >= (local_opts.saturation_value * 0.99));
                 double total_pixels = active_img.total();
                 double saturation_ratio = (total_pixels > 0) ? static_cast<double>(saturated_pixels) / total_pixels : 0.0;
                 pa_result.has_saturated_pixels = (saturation_ratio > DynaRange::Setup::Constants::MAX_PRE_ANALYSIS_SATURATION_RATIO);
                 pa_result.saturation_value_used = local_opts.saturation_value;
             }
         }
    }

    const DynaRange::Engine::Initialization::ConfigReporter reporter;
    reporter.PrintPreAnalysisTable(initial_file_info_vec, log_stream);

    FileOrderResult order = DetermineFileOrder(initial_file_info_vec, log_stream);
    local_opts.input_files = order.sorted_filenames;
    local_opts.plot_labels = GeneratePlotLabels(order.sorted_filenames, initial_file_info_vec, order.was_exif_sort_possible);

    // Reorder the loaded_raw_files vector to match the final sorted order
    std::vector<RawFile> sorted_loaded_files;
    sorted_loaded_files.reserve(order.sorted_filenames.size());
    // Reorder pre_analysis_results to match the final sorted order too
    std::vector<PreAnalysisResult> sorted_pre_analysis_results; // <<-- NUEVO VECTOR ORDENADO
    sorted_pre_analysis_results.reserve(order.sorted_filenames.size()); // <<-- RESERVAR ESPACIO

    for (const auto& filename : order.sorted_filenames) {
        // Mover RawFile
        auto raw_it = std::find_if(loaded_raw_files.begin(), loaded_raw_files.end(),
            [&](RawFile& file){ return file.GetFilename() == filename; });
        if (raw_it != loaded_raw_files.end()) {
            sorted_loaded_files.push_back(std::move(*raw_it));
        }
        // Copiar PreAnalysisResult
        auto pa_it = std::find_if(pre_analysis_results.begin(), pre_analysis_results.end(),
            [&](const PreAnalysisResult& pa){ return pa.filename == filename; }); // <<-- BUSCAR EN pre_analysis_results
        if (pa_it != pre_analysis_results.end()) { // <<-- SI SE ENCUENTRA
            sorted_pre_analysis_results.push_back(*pa_it); // <<-- AÑADIR AL VECTOR ORDENADO
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

    reporter.PrintFinalConfiguration(local_opts, result.bayer_pattern, log_stream);

    // Llamar a SelectPreAnalysisRawIndex con el vector ordenado de PreAnalysisResult
    int source_image_index = DynaRange::Engine::Initialization::SelectPreAnalysisRawIndex(
        sorted_pre_analysis_results, // <<-- PASAR EL VECTOR ORDENADO
        log_stream
    );

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