// File: src/core/setup/MetadataExtractor.hpp
/**
 * @file MetadataExtractor.hpp
 * @brief Declares the functionality for extracting key metadata from RAW files.
 * @details This module's single responsibility is to read raw files and produce
 * a structured list of their essential metadata for further processing.
 */
#pragma once
#include "raw/RawFile.hpp"
#include <string>
#include <vector>
#include <ostream>

/**
 * @struct FileInfo
 * @brief Holds extracted metadata for a single RAW file. This struct serves
 * as a data carrier between different setup stages.
 */
struct FileInfo {
    std::string filename;
    double mean_brightness = 0.0;
    float iso_speed = 0.0f;
};


/**
 * @brief Extracts metadata and loads RawFile objects in parallel.
 * @param input_files The list of input file paths.
 * @param log_stream Stream for logging messages.
 * @return A pair containing:
 * 1. A vector of FileInfo structs for each successfully processed file.
 * 2. A vector of the fully loaded RawFile objects.
 */
std::pair<std::vector<FileInfo>, std::vector<RawFile>> ExtractFileInfo(const std::vector<std::string>& input_files, std::ostream& log_stream);
