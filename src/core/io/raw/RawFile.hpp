// File: src/core/io/raw/RawFile.hpp
/**
 * @file src/core/io/raw/RawFile.hpp
 * @brief Defines the RawFile facade class for handling RAW image files.
 */
#pragma once

#include <string>
#include <memory>
#include <optional>
#include <opencv2/core/mat.hpp>

// Forward declarations
class LibRaw;
namespace DynaRange::IO::Raw {
    class RawImageAccessor;
    class RawMetadataExtractor;
}

/**
 * @class RawFile
 * @brief Manages the loading and data access of a single RAW image file.
 * @details This class acts as a Facade, providing a simple, unified interface
 * to the more complex underlying system of loading, data access, and metadata
 * extraction, which are handled by specialized helper classes.
 * It is a move-only type due to its ownership of unique resources.
 */
class RawFile {
public:
    explicit RawFile(std::string filename);
    ~RawFile();

    // --- Rule of Five: Make the class move-only ---
    RawFile(const RawFile&) = delete;
    RawFile& operator=(const RawFile&) = delete;
    RawFile(RawFile&&) noexcept;
    RawFile& operator=(RawFile&&) noexcept;
    
    bool Load();
    
    // --- Image Data Accessors (delegated) ---
    cv::Mat GetRawImage() const;
    cv::Mat GetActiveRawImage() const;
    cv::Mat GetProcessedImage();

    // --- Metadata Getters (delegated) ---
    std::string GetCameraModel() const;
    int GetWidth() const;
    int GetHeight() const;
    const std::string& GetFilename() const;
    bool IsLoaded() const;
    float GetIsoSpeed() const;
    double GetSensorResolutionMPx() const;
    int GetBlackLevelFromMetadata() const;
    int GetActiveWidth() const;
    int GetActiveHeight() const;
    int GetTopMargin() const;
    int GetLeftMargin() const;
    std::optional<int> GetBitDepth() const;

private:
    std::string m_filename;
    bool m_is_loaded = false;
    
    // LibRaw instance shared between helpers
    std::shared_ptr<LibRaw> m_raw_processor;

    // Specialized helper components
    std::unique_ptr<DynaRange::IO::Raw::RawImageAccessor> m_image_accessor;
    std::unique_ptr<DynaRange::IO::Raw::RawMetadataExtractor> m_metadata_extractor;
};