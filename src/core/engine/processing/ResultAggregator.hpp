// File: src/core/engine/processing/ResultAggregator.hpp
/**
 * @file ResultAggregator.hpp
 * @brief Implements the analysis result aggregation logic.
 */
/**
 * @file ResultAggregator.hpp
 * @brief Declares the function for aggregating and finalizing analysis results.
 * @details This module's responsibility is to take the raw patch analysis data
 * from individual Bayer channels and format it into the final, user-facing
 * result structures, adhering to SRP.
 */
#pragma once

#include "Processing.hpp" // For SingleFileResult
#include "../../analysis/Analysis.hpp"
#include "../../io/raw/RawFile.hpp"
#include <vector>
#include <map>
#include <ostream>
#include <mutex>

namespace DynaRange::Engine::Processing {

/**
 * @brief Aggregates patch data from individual channels and finalizes the results.
 * @param individual_channel_patches A map containing the PatchAnalysisResult for each analyzed channel.
 * @param raw_file The source RawFile object, used for metadata.
 * @param params The consolidated analysis parameters.
 * @param generate_debug_image A reference to a flag controlling debug image creation.
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
    std::mutex& log_mutex
);
} // namespace DynaRange::Engine::Processing