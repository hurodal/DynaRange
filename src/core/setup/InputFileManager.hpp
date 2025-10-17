// File: src/core/setup/InputFileManager.hpp
/**
 * @file InputFileManager.hpp
 * @brief Declares a class to manage the application's input file lists.
 * @details This class provides a centralized, stateful manager for the main RAW
 * input files and the dedicated calibration files. It ensures that lists are
 * always clean, free of duplicates, and that calibration files are not
 * present in the main input list. It is designed to be used by both the
 * CLI and GUI to enforce the DRY principle.
 */
#pragma once

#include <string>
#include <vector>
#include <optional>

class InputFileManager {
public:
    /**
     * @brief Adds a list of files to the main input list.
     * @details Duplicates and files currently used for calibration are ignored.
     * @param files The vector of file paths to add.
     */
    void AddFiles(const std::vector<std::string>& files);

    /**
     * @brief Removes a list of files from the main input list.
     * @param files_to_remove The vector of file paths to remove.
     */
    void RemoveFiles(const std::vector<std::string>& files_to_remove);

    /**
     * @brief Sets the path for the dark frame file.
     * @details If the file was present in the main input list, it is removed from it.
     * @param file The path to the black file. An empty string clears it.
     */
    void SetBlackFile(const std::string& file);

    /**
     * @brief Sets the path for the saturation frame file.
     * @details If the file was present in the main input list, it is removed from it.
     * @param file The path to the saturation file. An empty string clears it.
     */
    void SetSaturationFile(const std::string& file);

    /**
     * @brief Gets the clean, sorted list of input files.
     * @return A constant reference to the vector of input file paths.
     */
    const std::vector<std::string>& GetInputFiles() const;

    /**
     * @brief Gets the path to the current black file.
     * @return An optional containing the path, or std::nullopt if not set.
     */
    std::optional<std::string> GetBlackFile() const;

    /**
     * @brief Gets the path to the current saturation file.
     * @return An optional containing the path, or std::nullopt if not set.
     */
    std::optional<std::string> GetSaturationFile() const;

private:
    void SanitizeInputFiles();

    std::vector<std::string> m_inputFiles;
    std::optional<std::string> m_blackFile;
    std::optional<std::string> m_saturationFile;
};