// File: src/core/engine/processing/Processing.cpp
/**
 * @file core/engine/processing/Processing.cpp
 * @brief Implements the core logic for processing and analyzing RAW files.
 */
#include "Processing.hpp"
#include "CornerDetectionHandler.hpp"
#include "ResultAggregator.hpp"
#include "../PatchAnalysisStrategy.hpp"
#include "../../analysis/Constants.hpp"
#include "../../io/RawFile.hpp"
#include "../../graphics/ImageProcessing.hpp"
#include "../../setup/ChartProfile.hpp"
#include "../../utils/PathManager.hpp"
#include "../../io/OutputWriter.hpp"
#include "../../graphics/geometry/KeystoneCorrection.hpp"
#include <filesystem>
#include <iostream>
#include <atomic>
#include <libintl.h>

#define _(string) gettext(string)
namespace fs = std::filesystem;

namespace { // Anonymous namespace for internal helper functions

std::vector<RawFile> LoadRawFiles(const std::vector<std::string>& input_files, std::ostream& log_stream) {
    std::vector<RawFile> raw_files;
    raw_files.reserve(input_files.size());
    for(const auto& filename : input_files) {
        raw_files.emplace_back(filename);
        if (!raw_files.back().Load()) {
            log_stream << _("Error: Could not load RAW file: ") << filename << std::endl;
        }
    }
    return raw_files;
}

std::vector<SingleFileResult> AnalyzeSingleRawFile(
    const RawFile& raw_file,
    const ProgramOptions& opts,
    const ChartProfile& chart,
    const Eigen::VectorXd& keystone_params,
    std::ostream& log_stream,
    double camera_resolution_mpx,
    bool generate_debug_image)
{
    log_stream << _("Processing \"") << fs::path(raw_file.GetFilename()).filename().string() << "\"..." << std::endl;
    std::map<DataSource, PatchAnalysisResult> individual_channel_patches;

    // --- 1. PREPARE FOR ANALYSIS ---
    double norm_adjustment = 0.0;
    if (opts.dr_normalization_mpx > 0.0 && camera_resolution_mpx > 0.0) {
        norm_adjustment = 20.0 * std::log10(std::sqrt(camera_resolution_mpx / opts.dr_normalization_mpx));
    }
    const double strict_min_snr_db = -10.0 - norm_adjustment;
    const double permissive_min_snr_db = DynaRange::Analysis::Constants::MIN_SNR_DB_THRESHOLD - norm_adjustment;
    
    double max_requested_threshold = 0.0;
    if (!opts.snr_thresholds_db.empty()) {
        max_requested_threshold = *std::max_element(opts.snr_thresholds_db.begin(), opts.snr_thresholds_db.end());
    }

    std::vector<DataSource> channels_to_analyze;
    if (opts.raw_channels.AVG) {
        channels_to_analyze = {DataSource::R, DataSource::G1, DataSource::G2, DataSource::B};
    } else {
        if (opts.raw_channels.R) channels_to_analyze.push_back(DataSource::R);
        if (opts.raw_channels.G1) channels_to_analyze.push_back(DataSource::G1);
        if (opts.raw_channels.G2) channels_to_analyze.push_back(DataSource::G2);
        if (opts.raw_channels.B) channels_to_analyze.push_back(DataSource::B);
    }

    // --- 2. CORE ANALYSIS LOOP ---
    for (const auto& channel : channels_to_analyze) {
        cv::Mat img_prepared = PrepareChartImage(raw_file, opts, keystone_params, chart, log_stream, channel);
        if (img_prepared.empty()) {
            log_stream << _("Error: Failed to prepare image for channel: ") << Formatters::DataSourceToString(channel) << std::endl;
            continue;
        }

        bool should_draw_overlay = generate_debug_image && (channel == DataSource::R);
        
        individual_channel_patches[channel] = DynaRange::Engine::PerformTwoPassPatchAnalysis(
            img_prepared, channel, chart, opts, log_stream,
            strict_min_snr_db, permissive_min_snr_db, max_requested_threshold, should_draw_overlay
        );
    }

    // --- 3. AGGREGATE AND FINALIZE RESULTS ---
    return DynaRange::Engine::Processing::AggregateAndFinalizeResults(individual_channel_patches, raw_file, opts, camera_resolution_mpx, generate_debug_image, log_stream);
}

} // end of anonymous namespace

ProcessingResult ProcessFiles(const ProgramOptions& opts, std::ostream& log_stream, const std::atomic<bool>& cancel_flag) {
    ProcessingResult result;
    // 1. Load files
    std::vector<RawFile> raw_files = LoadRawFiles(opts.input_files, log_stream);
    PathManager paths(opts);

    // 2. Attempt automatic corner detection
    std::optional<std::vector<cv::Point2d>> detected_corners_opt = DynaRange::Engine::Processing::AttemptAutomaticCornerDetection(raw_files, opts, paths, log_stream);

    // 3. Define the analysis context
    ChartProfile chart(opts, detected_corners_opt, log_stream);
    std::string camera_model_name;
    if(!raw_files.empty() && raw_files[0].IsLoaded()){
        camera_model_name = raw_files[0].GetCameraModel();
    }

    // 4. Orchestrate analysis for each file.
    log_stream << _("Analyzing chart using a grid of ") << chart.GetGridCols() << _(" columns by ") << chart.GetGridRows() << _(" rows.") << std::endl;
    if (!opts.print_patch_filename.empty()) {
        fs::path debug_path = paths.GetCsvOutputPath().parent_path() / opts.print_patch_filename;
        log_stream << _("Debug patch image will be saved to: ") << debug_path.string() << std::endl;
    }
    log_stream << _("Starting Dynamic Range calculation process...") << std::endl;

    Eigen::VectorXd keystone_params;
    if (DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION) {
        log_stream <<  _("Using optimized keystone: calculating parameters once for the series...") << std::endl;
        keystone_params = DynaRange::Graphics::Geometry::CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
    }

    for (size_t i = 0; i < raw_files.size(); ++i) {
        if (cancel_flag) return {};
        const auto& raw_file = raw_files[i];
        if (!raw_file.IsLoaded()) continue;

        if (!DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION) {
            log_stream << "Using non-optimized keystone: recalculating parameters for each image..." << std::endl;
            keystone_params = DynaRange::Graphics::Geometry::CalculateKeystoneParams(chart.GetCornerPoints(), chart.GetDestinationPoints());
        }

        bool generate_debug_image = (i == 0 && !opts.print_patch_filename.empty());
        auto file_results_vec = AnalyzeSingleRawFile(raw_file, opts, chart, keystone_params, log_stream, opts.sensor_resolution_mpx, generate_debug_image);

        for(auto& file_result : file_results_vec) {
            if (!file_result.final_debug_image.empty()) {
                result.debug_patch_image = file_result.final_debug_image;
                fs::path debug_path = paths.GetCsvOutputPath().parent_path() / opts.print_patch_filename;
                OutputWriter::WriteDebugImage(file_result.final_debug_image, debug_path, log_stream);
            }

            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    }
    return result;
}