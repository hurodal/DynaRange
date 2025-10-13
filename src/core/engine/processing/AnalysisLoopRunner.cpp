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
#include <future> 
#include <mutex>
#include <opencv2/core.hpp>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace { // Anonymous namespace for internal helper functions

/**
 * @brief Analyzes a single RAW file to extract SNR and DR data.
 * @details This function orchestrates the analysis for one file, including
 * preparing the image, running the two-pass patch analysis for each
 * required channel, and aggregating the final results.
 * @param raw_file The loaded RawFile object to analyze.
 * @param opts The program configuration options.
 * @param chart The geometric profile of the test chart.
 * @param keystone_params The pre-calculated keystone correction parameters.
 * @param log_stream The stream for logging messages.
 * @param camera_resolution_mpx The camera's sensor resolution in megapixels.
 * @param generate_debug_image Flag to indicate if a debug overlay image should be created.
 * @param cancel_flag The atomic flag to check for cancellation requests.
 * @param log_mutex A mutex to synchronize access to the log_stream.
 * @return A vector of SingleFileResult structs, one for each channel analyzed (e.g., R, G1, G2, B, AVG).
 */
std::vector<SingleFileResult> AnalyzeSingleRawFile(
    const RawFile& raw_file,
    const ProgramOptions& opts,
    const ChartProfile& chart,
    const cv::Mat& keystone_params,
    std::ostream& log_stream,
    double camera_resolution_mpx,
    bool generate_debug_image,
    const std::atomic<bool>& cancel_flag,
    std::mutex& log_mutex)
{
    // Log the start of processing for this file.
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        log_stream << _("Processing \"") << fs::path(raw_file.GetFilename()).filename().string() << "\"..." << std::endl;
    }

    if (cancel_flag) return {};
    
    std::map<DataSource, PatchAnalysisResult> individual_channel_patches;

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
    if (opts.raw_channels.avg_mode != AvgMode::None) {
        channels_to_analyze = {DataSource::R, DataSource::G1, DataSource::G2, DataSource::B};
    } else {
        if (opts.raw_channels.R) channels_to_analyze.push_back(DataSource::R);
        if (opts.raw_channels.G1) channels_to_analyze.push_back(DataSource::G1);
        if (opts.raw_channels.G2) channels_to_analyze.push_back(DataSource::G2);
        if (opts.raw_channels.B) channels_to_analyze.push_back(DataSource::B);
    }

    for (const auto& channel : channels_to_analyze) {
        if (cancel_flag) return {};
        cv::Mat img_prepared = PrepareChartImage(raw_file, opts, keystone_params, chart, log_stream, channel);
        if (img_prepared.empty()) {
            std::lock_guard<std::mutex> lock(log_mutex);
            log_stream << _("Error: Failed to prepare image for channel: ") << Formatters::DataSourceToString(channel) << " for file " << raw_file.GetFilename() << std::endl;
            continue;
        }

        bool should_draw_overlay = generate_debug_image && (channel == DataSource::R);
        individual_channel_patches[channel] = DynaRange::Engine::PerformTwoPassPatchAnalysis(
            img_prepared, channel, chart, opts, log_stream,
            strict_min_snr_db, permissive_min_snr_db, max_requested_threshold, should_draw_overlay,
            log_mutex
        );
    }

    auto results = DynaRange::Engine::Processing::AggregateAndFinalizeResults(individual_channel_patches, raw_file, opts, camera_resolution_mpx, generate_debug_image, log_stream, log_mutex);

    {
        std::lock_guard<std::mutex> lock(log_mutex);
        log_stream << _("Processed \"") << fs::path(raw_file.GetFilename()).filename().string() << "\"." << std::endl;
        log_stream.flush(); 
    }

    return results;
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
    
    std::mutex log_mutex;

    cv::Mat keystone_params;
    bool optimized = DynaRange::EngineConfig::OPTIMIZE_KEYSTONE_CALCULATION;
    if (optimized) {
        std::lock_guard<std::mutex> lock(log_mutex);
        m_log_stream <<  _("Using optimized keystone: calculating parameters once for the series...") << std::endl;
        keystone_params = DynaRange::Graphics::Geometry::CalculateKeystoneParams(m_chart.GetCornerPoints(), m_chart.GetDestinationPoints());
    }

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 1; 
    }

    for (size_t i = 0; i < m_raw_files.size(); i += num_threads) {
        if (m_cancel_flag) break;
        
        std::vector<std::future<std::vector<SingleFileResult>>> batch_futures;

        for (size_t j = i; j < std::min(i + num_threads, m_raw_files.size()); ++j) {
            const auto& raw_file = m_raw_files[j];
            if (!raw_file.IsLoaded()) continue;

            bool generate_debug_image = (j == 0 && !m_opts.print_patch_filename.empty());
            
            batch_futures.push_back(std::async(std::launch::async, 
                [&, generate_debug_image, keystone_params, &raw_file = raw_file]() {
                    cv::Mat local_keystone = keystone_params;
                    if (!optimized) {
                        local_keystone = DynaRange::Graphics::Geometry::CalculateKeystoneParams(m_chart.GetCornerPoints(), m_chart.GetDestinationPoints());
                    }
                    return AnalyzeSingleRawFile(raw_file, m_opts, m_chart, local_keystone, m_log_stream, m_opts.sensor_resolution_mpx, generate_debug_image, m_cancel_flag, log_mutex);
                }
            ));
        }
        
        for (auto& fut : batch_futures) {
            if (m_cancel_flag) break;
            
            std::vector<SingleFileResult> file_results_vec = fut.get();
            for (auto& file_result : file_results_vec) {
                if (!file_result.final_debug_image.empty()) {
                    std::lock_guard<std::mutex> lock(log_mutex);
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
    }
    
    return result;
}

} // namespace DynaRange::Engine::Processing