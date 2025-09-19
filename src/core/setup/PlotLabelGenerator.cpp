// File: src/core/setup/PlotLabelGenerator.cpp
/**
 * @file PlotLabelGenerator.cpp
 * @brief Implements the plot label generation logic.
 */
#include "PlotLabelGenerator.hpp"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

std::map<std::string, std::string> GeneratePlotLabels(
    const std::vector<std::string>& sorted_filenames,
    const std::vector<FileInfo>& original_file_info,
    bool was_exif_sort_possible
) {
    std::map<std::string, std::string> labels;
    
    // Create a quick lookup map from filename to its metadata
    std::map<std::string, FileInfo> info_map;
    for(const auto& info : original_file_info) {
        info_map[info.filename] = info;
    }

    for (const auto& filename : sorted_filenames) {
        if (was_exif_sort_possible) {
            std::stringstream label_ss;
            label_ss << "ISO " << static_cast<int>(info_map.at(filename).iso_speed);
            labels[filename] = label_ss.str();
        } else {
            labels[filename] = fs::path(filename).stem().string();
        }
    }
    return labels;
}