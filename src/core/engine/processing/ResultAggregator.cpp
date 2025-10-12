// File: src/core/engine/processing/ResultAggregator.cpp
/**
 * @file ResultAggregator.cpp
 * @brief Implements the analysis result aggregation logic.
 */
#include "ResultAggregator.hpp"
#include "../../graphics/ImageProcessing.hpp"
#include <libintl.h>
#include <filesystem>
#include <mutex>

#define _(string) gettext(string)
namespace fs = std::filesystem;

namespace DynaRange::Engine::Processing {

std::vector<SingleFileResult> AggregateAndFinalizeResults(
    const std::map<DataSource, PatchAnalysisResult>& individual_channel_patches,
    const RawFile& raw_file,
    const ProgramOptions& opts,
    double camera_resolution_mpx,
    bool& generate_debug_image,
    std::ostream& log_stream,
    std::mutex& log_mutex)
{
    std::vector<SingleFileResult> final_results;
    std::vector<DataSource> user_selected_channels;
    if (opts.raw_channels.R) user_selected_channels.push_back(DataSource::R);
    if (opts.raw_channels.G1) user_selected_channels.push_back(DataSource::G1);
    if (opts.raw_channels.G2) user_selected_channels.push_back(DataSource::G2);
    if (opts.raw_channels.B) user_selected_channels.push_back(DataSource::B);
    if (opts.raw_channels.AVG) user_selected_channels.push_back(DataSource::AVG);
    
    for (const auto& final_channel : user_selected_channels) {
        PatchAnalysisResult final_patch_data;
        if (final_channel == DataSource::AVG) {
            std::vector<DataSource> channels_to_pool = {DataSource::R, DataSource::G1, DataSource::G2, DataSource::B};
            for (const auto& channel_to_pool : channels_to_pool) {
                if (individual_channel_patches.count(channel_to_pool)) {
                    const auto& patch_result = individual_channel_patches.at(channel_to_pool);
                    final_patch_data.signal.insert(final_patch_data.signal.end(), patch_result.signal.begin(), patch_result.signal.end());
                    final_patch_data.noise.insert(final_patch_data.noise.end(), patch_result.noise.begin(), patch_result.noise.end());
                    final_patch_data.channels.insert(final_patch_data.channels.end(), patch_result.signal.size(), channel_to_pool);
                }
            }
        } else {
            if (individual_channel_patches.count(final_channel)) {
                final_patch_data = individual_channel_patches.at(final_channel);
            }
        }

        if (final_patch_data.signal.empty()) continue;
        
        auto [dr_result, curve_data] = CalculateResultsFromPatches(final_patch_data, opts, raw_file.GetFilename(), camera_resolution_mpx, final_channel);

        dr_result.iso_speed = raw_file.GetIsoSpeed();
        dr_result.samples_R = individual_channel_patches.count(DataSource::R) ? individual_channel_patches.at(DataSource::R).signal.size() : 0;
        dr_result.samples_G1 = individual_channel_patches.count(DataSource::G1) ? individual_channel_patches.at(DataSource::G1).signal.size() : 0;
        dr_result.samples_G2 = individual_channel_patches.count(DataSource::G2) ? individual_channel_patches.at(DataSource::G2).signal.size() : 0;
        dr_result.samples_B = individual_channel_patches.count(DataSource::B) ? individual_channel_patches.at(DataSource::B).signal.size() : 0;
        
        if(opts.plot_labels.count(raw_file.GetFilename())) {
            curve_data.plot_label = opts.plot_labels.at(raw_file.GetFilename());
        } else {
            curve_data.plot_label = fs::path(raw_file.GetFilename()).stem().string();
        }
        curve_data.iso_speed = raw_file.GetIsoSpeed();

        cv::Mat final_debug_image;
        if (generate_debug_image) {
            if (individual_channel_patches.count(DataSource::R)) {
                const auto& r_patches = individual_channel_patches.at(DataSource::R);
                final_debug_image = CreateFinalDebugImage(r_patches.image_with_patches, r_patches.max_pixel_value);
                if (final_debug_image.empty()) {
                    std::lock_guard<std::mutex> lock(log_mutex);
                    log_stream << "  - " << _("Warning: Could not generate debug patch image for this file.") << std::endl;
                }
            }
            generate_debug_image = false; // Ensure it's only generated once
        }

        final_results.push_back(SingleFileResult{dr_result, curve_data, final_debug_image});
    }
    return final_results;
}

} // namespace DynaRange::Engine::Processing