// File: src/core/setup/MetadataExtractor.cpp
/**
 * @file src/core/setup/MetadataExtractor.cpp
 * @brief Implements the metadata extraction logic for RAW files.
 */
#include "MetadataExtractor.hpp"
#include "../io/raw/RawFile.hpp"
#include "PreAnalysis.hpp"
#include <iostream>
#include <libintl.h>

#define _(string) gettext(string)

std::pair<std::vector<FileInfo>, std::vector<RawFile>> ExtractFileInfo(const std::vector<std::string>& input_files, std::ostream& log_stream)
{
    // For the CLI, we need a saturation value to check for saturated pixels.
    // We use a very high default value to effectively disable the check at this stage,
    // as the real saturation value is not known until later in the initialization phase.
    // The GUI will call PreAnalyzeRawFiles directly with the correct saturation value.
    const double CLI_DEFAULT_SATURATION = 1e9;
    auto pre_analysis_results = PreAnalyzeRawFiles(input_files, CLI_DEFAULT_SATURATION, &log_stream);
    if (pre_analysis_results.empty()) {
        // Return empty vectors using move semantics to avoid copy.
        return std::make_pair(std::vector<FileInfo>(), std::vector<RawFile>());
    }
    std::vector<FileInfo> file_info_list;
    std::vector<RawFile> loaded_raw_files;
    file_info_list.reserve(pre_analysis_results.size());
    loaded_raw_files.reserve(pre_analysis_results.size());
    for (const auto& result : pre_analysis_results) {
        FileInfo info;
        info.filename = result.filename;
        info.mean_brightness = result.mean_brightness;
        info.iso_speed = result.iso_speed;
        file_info_list.push_back(info);
        // We need to reload the file because the RawFile in PreAnalyzeRawFiles is destroyed.
        // This is a small inefficiency for the CLI, but it keeps the core logic clean.
        // The GUI can optimize this by caching the loaded RawFile objects.
        RawFile raw_file(result.filename);
        raw_file.Load(); // We assume it will load successfully as it did in pre-analysis.
        // Use move semantics to add to the vector, as RawFile is non-copyable.
        loaded_raw_files.push_back(std::move(raw_file));
    }
    // Return the pair using move semantics.
    return std::make_pair(std::move(file_info_list), std::move(loaded_raw_files));
}