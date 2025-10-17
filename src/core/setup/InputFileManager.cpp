// File: src/core/setup/InputFileManager.cpp
/**
 * @file InputFileManager.cpp
 * @brief Implements the InputFileManager class.
 */
#include "InputFileManager.hpp"
#include <set>
#include <algorithm>

void InputFileManager::AddFiles(const std::vector<std::string>& files) {
    std::set<std::string> existing_files(m_inputFiles.begin(), m_inputFiles.end());
    for (const auto& file : files) {
        if (existing_files.find(file) == existing_files.end()) {
            m_inputFiles.push_back(file);
            existing_files.insert(file);
        }
    }
    SanitizeInputFiles();
}

void InputFileManager::RemoveFiles(const std::vector<std::string>& files_to_remove) {
    std::set<std::string> to_remove_set(files_to_remove.begin(), files_to_remove.end());
    m_inputFiles.erase(
        std::remove_if(m_inputFiles.begin(), m_inputFiles.end(),
            [&](const std::string& file) {
                return to_remove_set.count(file) > 0;
            }),
        m_inputFiles.end()
    );
}

void InputFileManager::SetBlackFile(const std::string& file) {
    m_blackFile = file.empty() ? std::nullopt : std::optional<std::string>(file);
    SanitizeInputFiles();
}

void InputFileManager::SetSaturationFile(const std::string& file) {
    m_saturationFile = file.empty() ? std::nullopt : std::optional<std::string>(file);
    SanitizeInputFiles();
}

const std::vector<std::string>& InputFileManager::GetInputFiles() const {
    return m_inputFiles;
}

std::optional<std::string> InputFileManager::GetBlackFile() const {
    return m_blackFile;
}

std::optional<std::string> InputFileManager::GetSaturationFile() const {
    return m_saturationFile;
}

void InputFileManager::SanitizeInputFiles() {
    if (m_inputFiles.empty()) {
        return;
    }

    std::set<std::string> calibration_files;
    if (m_blackFile.has_value()) {
        calibration_files.insert(m_blackFile.value());
    }
    if (m_saturationFile.has_value()) {
        calibration_files.insert(m_saturationFile.value());
    }

    if (!calibration_files.empty()) {
        RemoveFiles(std::vector<std::string>(calibration_files.begin(), calibration_files.end()));
    }

    // Ensure the list is sorted for consistent behavior
    std::sort(m_inputFiles.begin(), m_inputFiles.end());
}