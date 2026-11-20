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
#include "../../analysis/Constants.hpp"   
#include "../../graphics/ImageProcessing.hpp"
#include "../../utils/Formatters.hpp"
#include <libintl.h>
#include <future> 
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <fstream>
#include <iomanip>
#include <locale>
#include <cmath>

#define _(string) gettext(string)

namespace fs = std::filesystem;
namespace { // Anonymous namespace for internal helper functions

/**
 * @brief Writes, for one RAW file, the keystone-corrected chart of each channel as a 16-bit
 * TIFF with untouched RAW values, plus a CSV with the patch rectangles used by the analysis
 * and plain statistics of those exact pixels.
 * @details File names: <raw stem>_grid_<CH>.tif and <raw stem>_grid_patches.csv in the
 * output directory. Patch rectangles replicate AnalyzePatches() (same patch_ratio and rounding);
 * x2/y2 are exclusive. Rows are lettered A, B, ... from the top; columns numbered from 1.
 */
void ExportChartGrid(
    const RawFile& raw_file,
    const AnalysisParameters& params,
    const ChartProfile& chart,
    const cv::Mat& keystone_params,
    const std::vector<DataSource>& channels,
    const PathManager& paths,
    std::ostream& log_stream,
    std::mutex& log_mutex)
{
    const std::string stem = fs::path(raw_file.GetFilename()).stem().string();
    const fs::path csv_path = paths.GetFullPath(stem + "_grid_patches.csv");
    std::ofstream csv(csv_path);
    csv.imbue(std::locale::classic());
    csv << std::setprecision(10);
    csv << "# DynaRange grid export: untouched RAW values, nearest-neighbour keystone (no interpolation)\n";
    csv << "# raw_file," << fs::path(raw_file.GetFilename()).filename().string() << "\n";
    csv << "# iso," << raw_file.GetIsoSpeed() << "\n";
    csv << "# black_level," << params.dark_value << "\n";
    csv << "# saturation_level," << params.saturation_value << "\n";
    csv << "# patch_ratio," << params.patch_ratio << "\n";
    csv << "# note: mean/std are plain statistics of the patch pixels (no reconstruction of noise clipped at 0)\n";
    csv << "channel,patch,row,col,x1,y1,x2,y2,pixels,mean_raw,std_raw,signal_norm,noise_norm,snr_db\n";

    const int NCOLS = chart.GetGridCols();
    const int NROWS = chart.GetGridRows();
    const double range = params.saturation_value - params.dark_value;

    for (const auto& channel : channels) {
        const std::string ch = Formatters::DataSourceToString(channel);
        cv::Mat grid = PrepareChartRawImage(raw_file, keystone_params, chart, channel);
        if (grid.empty()) {
            std::lock_guard<std::mutex> lock(log_mutex);
            log_stream << _("Warning: could not export grid image for channel ") << ch << std::endl;
            continue;
        }
        const fs::path tif_path = paths.GetFullPath(stem + "_grid_" + ch + ".tif");
        bool written = false;
        try {
            written = cv::imwrite(tif_path.string(), grid);
        } catch (const cv::Exception&) {
            written = false;
        }
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            if (written) log_stream << _("  - Grid image (RAW values) saved to: ") << tif_path.string() << std::endl;
            else log_stream << _("Warning: could not write grid image: ") << tif_path.string() << std::endl;
        }

        const double pw = static_cast<double>(grid.cols) / NCOLS;
        const double ph = static_cast<double>(grid.rows) / NROWS;
        const double safe_x = pw * (1.0 - params.patch_ratio) / 2.0;
        const double safe_y = ph * (1.0 - params.patch_ratio) / 2.0;
        for (int j = 0; j < NROWS; ++j) {
            for (int i = 0; i < NCOLS; ++i) {
                const int x1 = static_cast<int>(round(i * pw + safe_x));
                const int x2 = static_cast<int>(round((i + 1) * pw - safe_x));
                const int y1 = static_cast<int>(round(j * ph + safe_y));
                const int y2 = static_cast<int>(round((j + 1) * ph - safe_y));
                if (x1 >= x2 || y1 >= y2 || x1 < 0 || y1 < 0 || x2 > grid.cols || y2 > grid.rows) continue;
                cv::Scalar mean, stddev;
                cv::meanStdDev(grid(cv::Rect(x1, y1, x2 - x1, y2 - y1)), mean, stddev);
                const double S = (mean[0] - params.dark_value) / range;
                const double N = stddev[0] / range;
                const std::string label = std::string(1, static_cast<char>('A' + j)) + std::to_string(i + 1);
                csv << ch << "," << label << "," << (j + 1) << "," << (i + 1) << ","
                    << x1 << "," << y1 << "," << x2 << "," << y2 << "," << (x2 - x1) * (y2 - y1) << ","
                    << mean[0] << "," << stddev[0] << "," << S << "," << N << ",";
                if (S > 0.0 && N > 0.0) csv << 20.0 * std::log10(S / N);
                csv << "\n";
            }
        }
    }
}

std::vector<SingleFileResult> AnalyzeSingleRawFile(
    const RawFile& raw_file,
    const AnalysisParameters& params, // Contiene generate_full_debug
    const ChartProfile& chart,
    const cv::Mat& keystone_params,
    std::ostream& log_stream,
    bool generate_debug_image, // Este es para printpatches
    const std::atomic<bool>& cancel_flag,
    std::mutex& log_mutex,
    const PathManager& paths,
    const std::string& camera_model_name
)
{
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
        // *** PASAR params.generate_full_debug ***
        cv::Mat img_prepared = PrepareChartImage(
            raw_file,
            params.dark_value,
            params.saturation_value,
            keystone_params,
            chart,
            log_stream,
            channel,
            paths,
            camera_model_name,
            params.generate_full_debug // <-- Pasar el flag
        );
        if (img_prepared.empty()) {
            std::lock_guard<std::mutex> lock(log_mutex);
            log_stream << _("Error: Failed to prepare image for channel: ") << Formatters::DataSourceToString(channel) << " for file " << raw_file.GetFilename() << std::endl;
            continue;
        }

        bool should_draw_overlay = generate_debug_image && (channel == DataSource::G1);
        individual_channel_patches[channel] = DynaRange::Engine::PerformTwoPassPatchAnalysis(
            img_prepared, channel, chart, params.patch_ratio, log_stream,
            strict_min_snr_db, permissive_min_snr_db, max_requested_threshold, should_draw_overlay,
            log_mutex,
            params.dark_value
        );
    }

    if (params.export_grid && !cancel_flag) {
        ExportChartGrid(raw_file, params, chart, keystone_params, channels_to_analyze, paths, log_stream, log_mutex);
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
    int source_image_index,
    const PathManager& paths)
    : m_raw_files(raw_files),
      m_params(params),
      m_chart(chart),
      m_camera_model_name(camera_model_name),
      m_log_stream(log_stream),
      m_cancel_flag(cancel_flag),
      m_source_image_index(source_image_index),
      m_paths(paths)
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

            bool generate_debug_image = (j == m_source_image_index && !m_params.print_patch_filename.empty());
            batch_futures.push_back(std::async(std::launch::async,
                // Captura m_params por referencia porque AnalysisLoopRunner vive durante la ejecución
                [&, generate_debug_image, keystone_params, &raw_file = raw_file, camera_model = m_camera_model_name]() {
                    cv::Mat local_keystone = keystone_params;
                    if (!optimized) {
                        local_keystone = DynaRange::Graphics::Geometry::CalculateKeystoneParams(m_chart.GetCornerPoints(), m_chart.GetDestinationPoints());
                    }
                    // m_params ya contiene generate_full_debug
                    return AnalyzeSingleRawFile(raw_file, m_params, m_chart, local_keystone, m_log_stream, generate_debug_image, m_cancel_flag, log_mutex, m_paths, camera_model);
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