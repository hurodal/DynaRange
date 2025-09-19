// File: src/core/setup/PlotLabelGenerator.hpp
/**
 * @file PlotLabelGenerator.hpp
 * @brief Declares the functionality for generating plot labels for RAW files.
 * @details This module's single responsibility is to create a mapping of
 * filenames to human-readable labels for use in plots.
 */
#pragma once
#include "MetadataExtractor.hpp" // For FileInfo
#include <vector>
#include <string>
#include <map>

/**
 * @brief Generates plot labels for each RAW file.
 * @details Decides whether to use ISO-based labels (e.g., "ISO 100") or
 * filename-based labels, depending on the availability of EXIF data.
 * @param sorted_filenames The final, ordered list of filenames.
 * @param original_file_info The initial list of FileInfo to look up metadata.
 * @param was_exif_sort_possible A flag indicating if EXIF data was available for all files.
 * @return A map of filenames to their corresponding plot labels.
 */
std::map<std::string, std::string> GeneratePlotLabels(
    const std::vector<std::string>& sorted_filenames,
    const std::vector<FileInfo>& original_file_info,
    bool was_exif_sort_possible
);