// File: src/core/setup/MetadataExtractor.cpp
/**
 * @file MetadataExtractor.cpp
 * @brief Implements the metadata extraction logic from RAW files.
 */
#include "MetadataExtractor.hpp"
#include "../io/raw/RawFile.hpp"
#include <opencv2/imgproc.hpp>
#include <libintl.h>

#define _(string) gettext(string)

std::vector<FileInfo> ExtractFileInfo(const std::vector<std::string>& input_files, std::ostream& log_stream) {
    std::vector<FileInfo> file_info_list;

    for (const std::string& name : input_files) {
        RawFile raw_file(name);
        if (!raw_file.Load()) {
            continue;
        }

        FileInfo info;
        info.filename = name;

        // Get the active image area directly.
        cv::Mat active_img = raw_file.GetActiveRawImage();
        if (!active_img.empty()) {
            // The mean brightness is calculated only on the active pixels.
            info.mean_brightness = cv::mean(active_img)[0];
        }
        info.iso_speed = raw_file.GetIsoSpeed();

        file_info_list.push_back(info);
    }
    return file_info_list;
}