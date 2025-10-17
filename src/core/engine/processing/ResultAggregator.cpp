// File: src/core/engine/processing/ResultAggregator.cpp
/**
 * @file ResultAggregator.cpp
 * @brief Implements the analysis result aggregation logic.
 */
#include "ResultAggregator.hpp"
#include "../../graphics/ImageProcessing.hpp"
#include "../utils/Formatters.hpp"
#include <libintl.h>
#include <mutex>

#define _(string) gettext(string)

namespace DynaRange::Engine::Processing {

/**
 * @brief Aggregates patch data from individual channels and finalizes the results for a single RAW file.
 * @details This function takes the raw patch analysis from each Bayer channel, calculates the final
 * results for user-selected channels (including AVG), and prepares the SingleFileResult structs.
 * It is also responsible for triggering the creation of the final debug patch image.
 * @param individual_channel_patches A map containing the PatchAnalysisResult for each analyzed channel.
 * @param raw_file The source RawFile object, used for metadata.
 * @param params The consolidated analysis parameters.
 * @param generate_debug_image A reference to a flag controlling debug image creation. This flag will be set to false by this function.
 * @param log_stream The output stream for logging.
 * @param log_mutex A mutex to synchronize access to the log_stream.
 * @return A vector of SingleFileResult structs for the processed file.
 */
std::vector<SingleFileResult> AggregateAndFinalizeResults(
    const std::map<DataSource, PatchAnalysisResult>& individual_channel_patches,
    const RawFile& raw_file,
    const AnalysisParameters& params,
    bool& generate_debug_image,
    std::ostream& log_stream,
    std::mutex& log_mutex)
{
    std::vector<SingleFileResult> final_results;
    std::vector<DataSource> user_selected_channels;
    if (params.raw_channels.R) user_selected_channels.push_back(DataSource::R);
    if (params.raw_channels.G1) user_selected_channels.push_back(DataSource::G1);
    if (params.raw_channels.G2) user_selected_channels.push_back(DataSource::G2);
    if (params.raw_channels.B) user_selected_channels.push_back(DataSource::B);
    // First, process individual channels
    for (const auto& final_channel : user_selected_channels) {
        PatchAnalysisResult final_patch_data;
        if (individual_channel_patches.count(final_channel)) {
            final_patch_data = individual_channel_patches.at(final_channel);
        }

        if (final_patch_data.signal.empty()) continue;
        
        auto [dr_result, curve_data] = CalculateResultsFromPatches(final_patch_data, params, raw_file.GetFilename(), final_channel);
        dr_result.iso_speed = raw_file.GetIsoSpeed();
        dr_result.samples_R = individual_channel_patches.count(DataSource::R) ? individual_channel_patches.at(DataSource::R).signal.size() : 0;
        dr_result.samples_G1 = individual_channel_patches.count(DataSource::G1) ? individual_channel_patches.at(DataSource::G1).signal.size() : 0;
        dr_result.samples_G2 = individual_channel_patches.count(DataSource::G2) ? individual_channel_patches.at(DataSource::G2).signal.size() : 0;
        dr_result.samples_B = individual_channel_patches.count(DataSource::B) ? individual_channel_patches.at(DataSource::B).signal.size() : 0;
        if(params.plot_labels.count(raw_file.GetFilename())) {
            curve_data.plot_label = params.plot_labels.at(raw_file.GetFilename());
        } else {
            curve_data.plot_label = fs::path(raw_file.GetFilename()).stem().string();
        }
        curve_data.iso_speed = raw_file.GetIsoSpeed();

        cv::Mat final_debug_image;
        if (generate_debug_image) {
            if (individual_channel_patches.count(DataSource::G1)) {
                const auto& g1_patches = individual_channel_patches.at(DataSource::G1);
                
                // Add a log message if no patches were found, indicating the overlay will be empty.
                if (g1_patches.signal.empty()) {
                    std::lock_guard<std::mutex> lock(log_mutex);
                    log_stream << "  - Info: No valid patches found. Saving debug image without overlays." << std::endl;
                }

                final_debug_image = CreateFinalDebugImage(g1_patches.image_with_patches, g1_patches.max_pixel_value);
                if (final_debug_image.empty()) {
                    std::lock_guard<std::mutex> lock(log_mutex);
                    log_stream << "  - " << _("Warning: Could not generate debug patch image for this file.") << std::endl;
                }
            }
            generate_debug_image = false;
        }

        final_results.push_back(SingleFileResult{dr_result, curve_data, final_debug_image});
    }

    // Second, process the average channel if requested
    if (params.raw_channels.avg_mode != AvgMode::None) {
        PatchAnalysisResult final_patch_data;
        std::vector<DataSource> channels_to_pool;
        std::string plot_label_suffix;

        if (params.raw_channels.avg_mode == AvgMode::Full) {
            channels_to_pool = {DataSource::R, DataSource::G1, DataSource::G2, DataSource::B};
            plot_label_suffix = " (Full)";
        } else { // AvgMode::Selected
            channels_to_pool = user_selected_channels;
            plot_label_suffix = " (";
            for(size_t i = 0; i < channels_to_pool.size(); ++i) {
                plot_label_suffix += Formatters::DataSourceToString(channels_to_pool[i]) + (i < channels_to_pool.size() - 1 ? "," : "");
            }
            plot_label_suffix += ")";
        }

        for (const auto& channel_to_pool : channels_to_pool) {
            if (individual_channel_patches.count(channel_to_pool)) {
                const auto& patch_result = individual_channel_patches.at(channel_to_pool);
                final_patch_data.signal.insert(final_patch_data.signal.end(), patch_result.signal.begin(), patch_result.signal.end());
                final_patch_data.noise.insert(final_patch_data.noise.end(), patch_result.noise.begin(), patch_result.noise.end());
                final_patch_data.channels.insert(final_patch_data.channels.end(), patch_result.signal.size(), channel_to_pool);
            }
        }

        if (!final_patch_data.signal.empty()) {
            auto [dr_result, curve_data] = CalculateResultsFromPatches(final_patch_data, params, raw_file.GetFilename(), DataSource::AVG);
            dr_result.iso_speed = raw_file.GetIsoSpeed();
            dr_result.samples_R = individual_channel_patches.count(DataSource::R) ? individual_channel_patches.at(DataSource::R).signal.size() : 0;
            dr_result.samples_G1 = individual_channel_patches.count(DataSource::G1) ? individual_channel_patches.at(DataSource::G1).signal.size() : 0;
            dr_result.samples_G2 = individual_channel_patches.count(DataSource::G2) ? individual_channel_patches.at(DataSource::G2).signal.size() : 0;
            dr_result.samples_B = individual_channel_patches.count(DataSource::B) ? individual_channel_patches.at(DataSource::B).signal.size() : 0;
            
            curve_data.plot_label = "AVG" + plot_label_suffix;
            curve_data.iso_speed = raw_file.GetIsoSpeed();

            final_results.push_back(SingleFileResult{dr_result, curve_data, cv::Mat()});
        }
    }
    
    return final_results;
}
}