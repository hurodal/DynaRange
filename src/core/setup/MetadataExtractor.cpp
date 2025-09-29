// File: src/core/setup/MetadataExtractor.cpp
/**
 * @file MetadataExtractor.cpp
 * @brief Implements the metadata extraction logic from RAW files.
 */
#include "MetadataExtractor.hpp"
#include "../io/RawFile.hpp"
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

        cv::Mat raw_img = raw_file.GetRawImage();
        if (!raw_img.empty()) {
            info.mean_brightness = cv::mean(raw_img)[0];
        }
        info.iso_speed = raw_file.GetIsoSpeed();

        file_info_list.push_back(info);
    }
    return file_info_list;
}