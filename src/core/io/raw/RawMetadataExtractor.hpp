// File: src/core/io/raw/RawMetadataExtractor.hpp
/**
 * @file RawMetadataExtractor.hpp
 * @brief Declares a component for extracting metadata from a loaded RAW file.
 * @details This module adheres to SRP by handling the extraction of all metadata
 * fields (camera model, ISO, dimensions, etc.) from a LibRaw object.
 */
#pragma once

#include <libraw/libraw.h>
#include <string>
#include <memory>
#include <optional>

namespace DynaRange::IO::Raw {

/**
 * @class RawMetadataExtractor
 * @brief Extracts various metadata fields from a LibRaw object.
 */
class RawMetadataExtractor {
public:
    explicit RawMetadataExtractor(std::shared_ptr<LibRaw> raw_processor);

    std::string GetCameraModel() const;
    int GetWidth() const;
    int GetHeight() const;
    float GetIsoSpeed() const;
    double GetSensorResolutionMPx() const;
    int GetBlackLevelFromMetadata() const;
    int GetActiveWidth() const;
    int GetActiveHeight() const;
    int GetTopMargin() const;
    int GetLeftMargin() const;
    std::optional<int> GetBitDepth() const;

private:
    std::shared_ptr<LibRaw> m_raw_processor;
    mutable std::string m_camera_model_cache;
    mutable float m_iso_speed_cache = 0.0f;
};

} // namespace DynaRange::IO::Raw