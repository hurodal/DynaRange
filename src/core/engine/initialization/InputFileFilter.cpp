// File: src/core/engine/initialization/InputFileFilter.cpp
/**
 * @file InputFileFilter.cpp
 * @brief Implements the input file list filtering logic.
 */
#include "InputFileFilter.hpp"
#include <set>
#include <algorithm>
#include <filesystem>
#include <libintl.h>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace DynaRange::Engine::Initialization {

void InputFileFilter::Filter(ProgramOptions& opts, std::ostream& log_stream) const {
    // --- 1. Exclude Calibration Files from Analysis ---
    if (!opts.dark_file_path.empty() || !opts.sat_file_path.empty()) {
        std::set<std::string> calibration_files;
        if (!opts.dark_file_path.empty()) {
            calibration_files.insert(opts.dark_file_path);
        }
        if (!opts.sat_file_path.empty()) {
            calibration_files.insert(opts.sat_file_path);
        }

        std::vector<std::string> files_to_remove;
        opts.input_files.erase(
            std::remove_if(opts.input_files.begin(), opts.input_files.end(),
                [&](const std::string& input_file) {
                    if (calibration_files.count(input_file)) {
                        files_to_remove.push_back(input_file);
                        return true;
                    }
                    return false;
                }),
            opts.input_files.end()
        );
        if (!files_to_remove.empty()) {
            log_stream << _("[INFO] The following files were excluded from the analysis because they are used for calibration:") << std::endl;
            for (const auto& file : files_to_remove) {
                log_stream << "  - " << fs::path(file).filename().string() << std::endl;
            }
        }
    }

    // --- 2. Deduplicate Input Files ---
    if (!opts.input_files.empty()) {
        std::vector<std::string> unique_files;
        std::set<std::string> seen_files;
        unique_files.reserve(opts.input_files.size());
        for (const auto& file : opts.input_files) {
            if (seen_files.insert(file).second) {
                unique_files.push_back(file);
            } else {
                log_stream << _("Warning: Duplicate input file ignored: ") << file << std::endl;
            }
        }
        opts.input_files = unique_files;
    }
}

} // namespace DynaRange::Engine::Initialization