// File: src/core/engine/processing/ResultAggregator.hpp
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
#include "../../io/RawFile.hpp"
#include "../../arguments/ArgumentsOptions.hpp"
#include <vector>
#include <map>
#include <ostream>

namespace DynaRange::Engine::Processing {

/**
 * @brief Aggregates patch data from individual channels and finalizes the results.
 * @details Takes the results of the patch analysis for each Bayer channel, combines
 * them based on user selection (including AVG), calculates final DR values,
 * and formats the output into a vector of SingleFileResult structs.
 * @param individual_channel_patches A map containing the PatchAnalysisResult for each analyzed channel.
 * @param raw_file The source RawFile object, used for metadata.
 * @param opts The program options.
 * @param camera_resolution_mpx The camera's sensor resolution.
 * @param generate_debug_image A reference to a flag controlling debug image creation. The function will set it to false after use.
 * @param log_stream The output stream for logging.
 * @return A vector of SingleFileResult structs for the processed file.
 */
std::vector<SingleFileResult> AggregateAndFinalizeResults(
    const std::map<DataSource, PatchAnalysisResult>& individual_channel_patches,
    const RawFile& raw_file,
    const ProgramOptions& opts,
    double camera_resolution_mpx,
    bool& generate_debug_image,
    std::ostream& log_stream
);

} // namespace DynaRange::Engine::Processing