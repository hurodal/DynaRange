// File: src/core/setup/MetadataExtractor.cpp
/**
 * @file MetadataExtractor.cpp
 * @brief Implements the metadata extraction logic from RAW files.
 */
#include "MetadataExtractor.hpp"
#include "../io/raw/RawFile.hpp"
#include <opencv2/imgproc.hpp>
#include <libintl.h>
#include <future>
#include <mutex>
#include <filesystem>

namespace fs = std::filesystem;

#define _(string) gettext(string)

// The function now returns a pair: the FileInfo vector for sorting, and the vector of loaded RawFile objects.
std::pair<std::vector<FileInfo>, std::vector<RawFile>> ExtractFileInfo(const std::vector<std::string>& input_files, std::ostream& log_stream) {
    std::vector<std::future<std::pair<std::optional<FileInfo>, RawFile>>> futures;
    std::mutex log_mutex;

    auto process_single_file = 
        [&log_stream, &log_mutex](const std::string& name) -> std::pair<std::optional<FileInfo>, RawFile> {
        
        RawFile raw_file(name);
        if (!raw_file.Load()) {
            std::lock_guard<std::mutex> lock(log_mutex);
            log_stream << _("Warning: Could not pre-load RAW file for metadata extraction: ") << name << std::endl;
            return {std::nullopt, std::move(raw_file)};
        }

        FileInfo info;
        info.filename = name;

        cv::Mat active_img = raw_file.GetActiveRawImage();
        if (!active_img.empty()) {
            info.mean_brightness = cv::mean(active_img)[0];
        }
        info.iso_speed = raw_file.GetIsoSpeed();

        {
            std::lock_guard<std::mutex> lock(log_mutex);
            log_stream << _("Pre-analyzed file: ") << fs::path(name).filename().string() << std::endl;
        }

        return {info, std::move(raw_file)};
    };

    for (const std::string& name : input_files) {
        futures.push_back(std::async(std::launch::async, process_single_file, name));
    }

    std::vector<FileInfo> file_info_list;
    std::vector<RawFile> loaded_raw_files;
    for (auto& fut : futures) {
        auto [info_opt, raw_file] = fut.get();
        if (info_opt.has_value()) {
            file_info_list.push_back(*info_opt);
            loaded_raw_files.push_back(std::move(raw_file));
        }
    }
    
    return {file_info_list, std::move(loaded_raw_files)};
}