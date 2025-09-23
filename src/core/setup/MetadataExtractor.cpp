// File: src/core/setup/MetadataExtractor.cpp
/**
 * @file MetadataExtractor.cpp
 * @brief Implements the metadata extraction logic from RAW files.
 */
#include "MetadataExtractor.hpp"
#include "../io/RawFile.hpp"
#include <iomanip>
#include <filesystem>
#include <opencv2/imgproc.hpp>
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

std::vector<FileInfo> ExtractFileInfo(const std::vector<std::string>& input_files, std::ostream& log_stream) {
    log_stream << _("Pre-analyzing files to determine sorting order...") << std::endl;
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
        log_stream << _("  - File: ") << fs::path(name).filename().string()
                   << _(", Brightness: ") << std::fixed << std::setprecision(2) << info.mean_brightness
                   << _(", ISO: ") << info.iso_speed << std::endl;
    }
    return file_info_list;
}