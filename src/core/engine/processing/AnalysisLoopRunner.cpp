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
 * @param raw_file The loaded RawFile object to analyze.
 * @param params The consolidated analysis parameters.
 * @param chart The geometric profile of the test chart.
 * @param keystone_params The pre-calculated keystone correction parameters.
 * @param log_stream The stream for logging messages.
 * @param generate_debug_image Flag to indicate if a debug overlay image should be created.
 * @param cancel_flag The atomic flag to check for cancellation requests.
 * @param log_mutex A mutex to synchronize access to the log_stream.
 * @return A vector of SingleFileResult structs, one for each channel analyzed (e.g., R, G1, G2, B, AVG).
 */
std::vector<SingleFileResult> AnalyzeSingleRawFile(
    const RawFile& raw_file,
    const AnalysisParameters& params,
    const ChartProfile& chart,
    const cv::Mat& keystone_params,
    std::ostream& log_stream,
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
    if (params.dr_normalization_mpx > 0.0 && params.sensor_resolution_mpx > 0.0) {
        norm_adjustment = 20.0 * std::log10(std::sqrt(params.sensor_resolution_mpx / params.dr_normalization_mpx));
    }
    const double strict_min_snr_db = -10.0 - norm_adjustment;
    const double permissive_min_snr_db = DynaRange::Analysis::Constants::MIN_SNR_DB_THRESHOLD - norm_adjustment;
    
    double max_requested_threshold = 0.0;
    if (!params.snr_thresholds_db.empty()) {
        max_requested_threshold = *std::max_element(params.snr_thresholds_db.begin(), params.snr_thresholds_db.end());
    }

    std::vector<DataSource> channels_to_analyze;
    if (params.raw_channels.avg_mode != AvgMode::None) {
        channels_to_analyze = {DataSource::R, DataSource::G1, DataSource::G2, DataSource::B};
    } else {
        if (params.raw_channels.R) channels_to_analyze.push_back(DataSource::R);
        if (params.raw_channels.G1) channels_to_analyze.push_back(DataSource::G1);
        if (params.raw_channels.G2) channels_to_analyze.push_back(DataSource::G2);
        if (params.raw_channels.B) channels_to_analyze.push_back(DataSource::B);
    }

    for (const auto& channel : channels_to_analyze) {
        if (cancel_flag) return {};
        
        cv::Mat img_prepared = PrepareChartImage(raw_file, params.dark_value, params.saturation_value, keystone_params, chart, log_stream, channel);
        if (img_prepared.empty()) {
            std::lock_guard<std::mutex> lock(log_mutex);
            log_stream << _("Error: Failed to prepare image for channel: ") << Formatters::DataSourceToString(channel) << " for file " << raw_file.GetFilename() << std::endl;
            continue;
        }

        bool should_draw_overlay = generate_debug_image && (channel == DataSource::R);
        
        individual_channel_patches[channel] = DynaRange::Engine::PerformTwoPassPatchAnalysis(
            img_prepared, channel, chart, params.patch_ratio, log_stream,
            strict_min_snr_db, permissive_min_snr_db, max_requested_threshold, should_draw_overlay,
            log_mutex
        );
    }

    auto results = DynaRange::Engine::Processing::AggregateAndFinalizeResults(individual_channel_patches, raw_file, params, generate_debug_image, log_stream, log_mutex);
    
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        log_stream << _("Processed \"") << fs::path(raw_file.GetFilename()).filename().string() << "\"." << std::endl;
        log_stream.flush(); 
    }

    return results;
}
}
namespace DynaRange::Engine::Processing {

AnalysisLoopRunner::AnalysisLoopRunner(
    const std::vector<RawFile>& raw_files,
    const AnalysisParameters& params,
    const ChartProfile& chart,
    const std::string& camera_model_name,
    std::ostream& log_stream,
    const std::atomic<bool>& cancel_flag,
    int source_image_index) // New parameter
    : m_raw_files(raw_files),
      m_params(params),
      m_chart(chart),
      m_camera_model_name(camera_model_name),
      m_log_stream(log_stream),
      m_cancel_flag(cancel_flag),
      m_source_image_index(source_image_index) // Initialize new member
{}

ProcessingResult AnalysisLoopRunner::Run()
{
    ProcessingResult result;
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

            // Use the selected source_image_index to decide which iteration generates the debug image.
            bool generate_debug_image = (j == m_source_image_index && !m_params.print_patch_filename.empty());

            batch_futures.push_back(std::async(std::launch::async, 
                [&, generate_debug_image, keystone_params, &raw_file = raw_file]() {
                    cv::Mat local_keystone = keystone_params;
                    if (!optimized) {
                        local_keystone = DynaRange::Graphics::Geometry::CalculateKeystoneParams(m_chart.GetCornerPoints(), m_chart.GetDestinationPoints());
     
                    }
                    return AnalyzeSingleRawFile(raw_file, m_params, m_chart, local_keystone, m_log_stream, generate_debug_image, m_cancel_flag, log_mutex);
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
                    
                    ProgramOptions temp_opts;
                    temp_opts.output_filename = "results.csv"; // Dummy path
                    PathManager paths(temp_opts);
                    fs::path debug_path = paths.GetCsvOutputPath().parent_path() / m_params.print_patch_filename;
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