// File: src/core/setup/FileSorter.cpp
/**
 * @file FileSorter.cpp
 * @brief Implements the file sorting and comparison logic.
 */
#include "FileSorter.hpp"
#include <algorithm>
#include <iostream>

FileOrderResult DetermineFileOrder(const std::vector<FileInfo>& file_info_list, std::ostream& log_stream) {
    FileOrderResult result;
    constexpr bool USE_EXIF_SORT_DEFAULT = false;
    
    // Responsibility 1: Check if EXIF-based sorting is viable
    result.was_exif_sort_possible = true;
    for (const auto& info : file_info_list) {
        if (info.iso_speed <= 0) {
            result.was_exif_sort_possible = false;
            break;
        }
    }

    // Responsibility 2: Perform brightness-based sort (default)
    std::vector<FileInfo> list_a = file_info_list;
    std::sort(list_a.begin(), list_a.end(), [](const FileInfo& a, const FileInfo& b) {
        return a.mean_brightness < b.mean_brightness;
    });

    // Responsibility 3: Perform EXIF-based sort and compare if possible
    if (result.was_exif_sort_possible) {
        std::vector<FileInfo> list_b = file_info_list;
        std::sort(list_b.begin(), list_b.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.iso_speed < b.iso_speed;
        });

        bool lists_match = std::equal(list_a.begin(), list_a.end(), list_b.begin(),
                                      [](const FileInfo& a, const FileInfo& b){ return a.filename == b.filename; });
        if (lists_match) {
            log_stream << "Sorting by brightness and by ISO produce the same file order." << std::endl;
        } else {
            log_stream << "\n[WARNING] Sorting by brightness and by ISO produce DIFFERENT file orders." << std::endl;
        }
    } else {
        log_stream << "\n[WARNING] Cannot use EXIF data. ISO not available in all files. Using brightness sorting." << std::endl;
    }

    // Responsibility 4: Select the final list
    const std::vector<FileInfo>* final_sorted_list = &list_a;
    if (USE_EXIF_SORT_DEFAULT && result.was_exif_sort_possible) {
        // This logic path is currently inactive but kept for completeness
        log_stream << "Using final file order from: EXIF ISO (List B)" << std::endl;
    } else {
        log_stream << "Using final file order from: Image Brightness (List A)" << std::endl;
    }

    for (const auto& info : *final_sorted_list) {
        result.sorted_filenames.push_back(info.filename);
    }
    return result;
}