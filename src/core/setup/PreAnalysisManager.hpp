// File: src/core/setup/PreAnalysisManager.hpp
/**
 * @file src/core/setup/PreAnalysisManager.hpp
 * @brief Declares a stateful manager for pre-analyzed RAW file metadata.
 * @details This class maintains a cache of PreAnalysisResult objects and provides
 * efficient methods to add, remove, and query the best file for preview.
 * It is designed to be used by both the CLI (for a single run) and the GUI (for interactive use).
 */
#pragma once
#include "PreAnalysis.hpp"
#include <string>
#include <vector>
#include <optional>
#include <algorithm>

/**
 * @class PreAnalysisManager
 * @brief Manages a cache of pre-analyzed RAW file metadata.
 */
class PreAnalysisManager {
public:
    /**
     * @brief Adds a new file to the cache by analyzing it.
     * @param filepath The path to the RAW file.
     * @param saturation_value The sensor's saturation level.
     * @return true if the file was successfully analyzed and added, false otherwise.
     */
    bool AddFile(const std::string& filepath, double saturation_value);

    /**
     * @brief Removes a file from the cache.
     * @param filepath The path to the RAW file to remove.
     */
    void RemoveFile(const std::string& filepath);

    /**
     * @brief Clears all files from the cache.
     */
    void Clear();

    /**
     * @brief Gets the list of all files in the cache, sorted by brightness (darkest to brightest).
     * @return A vector of filenames.
     */
    std::vector<std::string> GetSortedFileList() const;

    /**
     * @brief Gets the best file for preview (brightest non-saturated, or darkest if all are saturated).
     * @return The filepath of the best file, or std::nullopt if the cache is empty.
     */
    std::optional<std::string> GetBestPreviewFile() const;

private:
    std::vector<PreAnalysisResult> m_cache;
};