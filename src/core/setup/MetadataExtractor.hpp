// File: src/core/setup/MetadataExtractor.hpp
/**
 * @file MetadataExtractor.hpp
 * @brief Declares the functionality for extracting key metadata from RAW files.
 * @details This module's single responsibility is to read raw files and produce
 * a structured list of their essential metadata for further processing.
 */
#pragma once
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
 * @brief Extracts metadata (brightness, ISO) from a list of RAW files.
 * @param input_files The list of input file paths.
 * @param log_stream Stream for logging messages.
 * @return A vector of FileInfo structs, one for each successfully processed file.
 */
std::vector<FileInfo> ExtractFileInfo(const std::vector<std::string>& input_files, std::ostream& log_stream);