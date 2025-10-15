// File: src/core/setup/PreAnalysisManager.cpp
/**
 * @file src/core/setup/PreAnalysisManager.cpp
 * @brief Implements the PreAnalysisManager class.
 */
#include "PreAnalysisManager.hpp"

bool PreAnalysisManager::AddFile(const std::string& filepath, double saturation_value) {
    auto results = PreAnalyzeRawFiles({filepath}, saturation_value, nullptr);
    if (results.empty()) {
        return false;
    }
    m_cache.push_back(results[0]);
    return true;
}

void PreAnalysisManager::RemoveFile(const std::string& filepath) {
    m_cache.erase(
        std::remove_if(m_cache.begin(), m_cache.end(),
            [&](const PreAnalysisResult& r) { return r.filename == filepath; }),
        m_cache.end()
    );
}

void PreAnalysisManager::Clear() {
    m_cache.clear();
}

std::vector<std::string> PreAnalysisManager::GetSortedFileList() const {
    auto sorted_cache = m_cache;
    std::sort(sorted_cache.begin(), sorted_cache.end(),
        [](const PreAnalysisResult& a, const PreAnalysisResult& b) {
            return a.mean_brightness < b.mean_brightness;
        });
    std::vector<std::string> filenames;
    for (const auto& r : sorted_cache) {
        filenames.push_back(r.filename);
    }
    return filenames;
}

std::optional<std::string> PreAnalysisManager::GetBestPreviewFile() const {
    if (m_cache.empty()) {
        return std::nullopt;
    }

    // Find the brightest non-saturated file.
    auto non_saturated_it = std::max_element(m_cache.begin(), m_cache.end(),
        [](const PreAnalysisResult& a, const PreAnalysisResult& b) {
            if (a.has_saturated_pixels != b.has_saturated_pixels) {
                return a.has_saturated_pixels; // Prefer non-saturated
            }
            return a.mean_brightness < b.mean_brightness;
        });

    if (!non_saturated_it->has_saturated_pixels) {
        return non_saturated_it->filename;
    }

    // If all are saturated, return the darkest.
    auto darkest_it = std::min_element(m_cache.begin(), m_cache.end(),
        [](const PreAnalysisResult& a, const PreAnalysisResult& b) {
            return a.mean_brightness < b.mean_brightness;
        });
    return darkest_it->filename;
}