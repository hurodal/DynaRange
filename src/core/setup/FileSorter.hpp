// File: src/core/setup/FileSorter.hpp
/**
 * @file FileSorter.hpp
 * @brief Declares the functionality for sorting RAW files based on metadata.
 * @details This module's single responsibility is to determine the definitive
 * processing order of input files by applying different sorting strategies.
 */
#pragma once
#include "MetadataExtractor.hpp" // For FileInfo
#include <vector>
#include <ostream>

/**
 * @struct FileOrderResult
 * @brief Holds the result of the file ordering process, including the final
 * list and a flag indicating if EXIF data was reliable.
 */
struct FileOrderResult {
    std::vector<std::string> sorted_filenames;
    bool was_exif_sort_possible;
};
    
/**
 * @brief Determines the final processing order of RAW files.
 * @details Sorts files by brightness and ISO, compares the results, and selects
 * the definitive order based on internal logic.
 * @param file_info_list A vector of FileInfo structs containing pre-extracted metadata.
 * @param log_stream Stream for logging messages.
 * @return A FileOrderResult struct containing the sorted filenames.
 */
FileOrderResult DetermineFileOrder(const std::vector<FileInfo>& file_info_list, std::ostream& log_stream);