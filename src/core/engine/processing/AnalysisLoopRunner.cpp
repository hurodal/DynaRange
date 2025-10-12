// File: src/core/engine/processing/AnalysisLoopRunner.cpp
/**
 * @file AnalysisLoopRunner.cpp
 * @brief Implements the main analysis loop runner.
 */
#include "AnalysisLoopRunner.hpp"
#include "../PatchAnalysisStrategy.hpp"
#include "ResultAggregator.hpp"
#include "../../graphics/geometry/KeystoneCorrection.hpp"
#include "../../utils/PathManager.hpp"
#include "../../io/OutputWriter.hpp"
#include "../../analysis/Constants.hpp"   
#include "../../graphics/ImageProcessing.hpp"
#include "../../utils/Formatters.hpp"
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace { // Anonymous namespace for internal helper functions

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

} // end anonymous namespace

namespace DynaRange::Engine::Processing {

AnalysisLoopRunner::AnalysisLoopRunner(
    const std::vector<RawFile>& raw_files,
    const ProgramOptions& opts,
    const ChartProfile& chart,
    const std::string& camera_model_name,
    std::ostream& log_stream,
    const std::atomic<bool>& cancel_flag)
    : m_raw_files(raw_files),
      m_opts(opts),
      m_chart(chart),
      m_camera_model_name(camera_model_name),
      m_log_stream(log_stream),
      m_cancel_flag(cancel_flag)
{}

ProcessingResult AnalysisLoopRunner::Run()
{
    ProcessingResult result;
    PathManager paths(m_opts);

    Eigen::VectorXd keystone_params;
    bool optimized = DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION;

    if (optimized) {
        m_log_stream <<  _("Using optimized keystone: calculating parameters once for the series...") << std::endl;
        keystone_params = DynaRange::Graphics::Geometry::CalculateKeystoneParams(m_chart.GetCornerPoints(), m_chart.GetDestinationPoints());
    }

    for (size_t i = 0; i < m_raw_files.size(); ++i) {
        if (m_cancel_flag) return {};
        const auto& raw_file = m_raw_files[i];
        if (!raw_file.IsLoaded()) continue;

        if (!optimized) {
            m_log_stream << "Using non-optimized keystone: recalculating parameters for each image..." << std::endl;
            keystone_params = DynaRange::Graphics::Geometry::CalculateKeystoneParams(m_chart.GetCornerPoints(), m_chart.GetDestinationPoints());
        }

        bool generate_debug_image = (i == 0 && !m_opts.print_patch_filename.empty());
        auto file_results_vec = AnalyzeSingleRawFile(raw_file, m_opts, m_chart, keystone_params, m_log_stream, m_opts.sensor_resolution_mpx, generate_debug_image);

        for(auto& file_result : file_results_vec) {
            if (!file_result.final_debug_image.empty()) {
                result.debug_patch_image = file_result.final_debug_image;
                fs::path debug_path = paths.GetCsvOutputPath().parent_path() / m_opts.print_patch_filename;
                OutputWriter::WriteDebugImage(file_result.final_debug_image, debug_path, m_log_stream);
            }

            if (!file_result.dr_result.filename.empty()) {
                file_result.curve_data.camera_model = m_camera_model_name;
                result.dr_results.push_back(file_result.dr_result);
                result.curve_data.push_back(file_result.curve_data);
            }
        }
    }
    return result;
}

} // namespace DynaRange::Engine::Processing