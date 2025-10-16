// File: src/core/engine/PatchAnalysisStrategy.cpp
/**
 * @file PatchAnalysisStrategy.cpp
 * @brief Implements the two-pass patch analysis strategy.
 */
#include "PatchAnalysisStrategy.hpp"
#include "../analysis/ImageAnalyzer.hpp"
#include "../utils/Formatters.hpp"
#include <libintl.h>
#include <mutex>

#define _(string) gettext(string)

namespace DynaRange::Engine {

PatchAnalysisResult PerformTwoPassPatchAnalysis(
    const cv::Mat& prepared_image,
    DataSource channel,
    const ChartProfile& chart,
    double patch_ratio,
    std::ostream& log_stream,
    double strict_min_snr_db,
    double permissive_min_snr_db,
    double max_requested_threshold,
    bool create_overlay_image,
    std::mutex& log_mutex,
    double dark_value)
{
    // --- Pass 1: Analyze with the strict threshold ---
    PatchAnalysisResult patch_data = AnalyzePatches(prepared_image, chart.GetGridCols(), chart.GetGridRows(), patch_ratio, create_overlay_image, strict_min_snr_db, dark_value);

    // --- Validation Step ---
    bool needs_reanalysis = false;
    if (!patch_data.signal.empty()) {
        double min_snr_found = 1e6;
        for (size_t i = 0; i < patch_data.signal.size(); ++i) {
            if (patch_data.signal[i] > 0 && patch_data.noise[i] > 0) {
                double snr = 20.0 * log10(patch_data.signal[i] / patch_data.noise[i]);
                if (snr < min_snr_found) {
                    min_snr_found = snr;
                }
            }
        }
        if (min_snr_found > max_requested_threshold) {
            needs_reanalysis = true;
        }
    }

    // --- Pass 2 (Conditional): Re-analyze with the permissive threshold ---
    if (needs_reanalysis) {
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            log_stream << "  - Info: Re-analyzing channel " << Formatters::DataSourceToString(channel) 
                       << " with permissive threshold to find low-SNR data."
                       << std::endl;
        }
        patch_data = AnalyzePatches(prepared_image, chart.GetGridCols(), chart.GetGridRows(), patch_ratio, create_overlay_image, permissive_min_snr_db, dark_value);
    }

    if (patch_data.signal.empty()) {
        std::lock_guard<std::mutex> lock(log_mutex);
        log_stream << _("Warning: No valid patches found for channel: ") << Formatters::DataSourceToString(channel) << std::endl;
    }
    
    return patch_data;
}

} // namespace DynaRange::Engine