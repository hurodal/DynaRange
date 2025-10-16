// File: src/core/io/raw/RawFile.cpp
/**
 * @file src/core/io/raw/RawFile.cpp
 * @brief Implements the RawFile facade class.
 */
#include "RawFile.hpp"
#include "RawLoader.hpp"
#include "RawImageAccessor.hpp"
#include "RawMetadataExtractor.hpp"
#include <utility>

RawFile::RawFile(std::string filename) : m_filename(std::move(filename)) {}

RawFile::~RawFile() = default;

RawFile::RawFile(RawFile&& other) noexcept = default;
RawFile& RawFile::operator=(RawFile&& other) noexcept = default;

bool RawFile::Load() {
    if (m_is_loaded) return true;

    m_raw_processor = DynaRange::IO::Raw::RawLoader::Load(m_filename);
    if (!m_raw_processor) {
        return false;
    }

    m_image_accessor = std::make_unique<DynaRange::IO::Raw::RawImageAccessor>(m_raw_processor);
    m_metadata_extractor = std::make_unique<DynaRange::IO::Raw::RawMetadataExtractor>(m_raw_processor);
    
    m_is_loaded = true;
    return true;
}

cv::Mat RawFile::GetRawImage() const {
    return m_is_loaded ? m_image_accessor->GetRawImage() : cv::Mat{};
}

cv::Mat RawFile::GetActiveRawImage() const {
    return m_is_loaded ? m_image_accessor->GetActiveRawImage() : cv::Mat{};
}

cv::Mat RawFile::GetProcessedImage() {
    return m_is_loaded ? m_image_accessor->GetProcessedImage() : cv::Mat{};
}

std::string RawFile::GetCameraModel() const {
    return m_is_loaded ? m_metadata_extractor->GetCameraModel() : "";
}

float RawFile::GetIsoSpeed() const {
    return m_is_loaded ? m_metadata_extractor->GetIsoSpeed() : 0.0f;
}

int RawFile::GetWidth() const {
    return m_is_loaded ? m_metadata_extractor->GetWidth() : 0;
}

int RawFile::GetHeight() const {
    return m_is_loaded ? m_metadata_extractor->GetHeight() : 0;
}

const std::string& RawFile::GetFilename() const {
    return m_filename;
}

bool RawFile::IsLoaded() const {
    return m_is_loaded;
}

double RawFile::GetSensorResolutionMPx() const {
    return m_is_loaded ? m_metadata_extractor->GetSensorResolutionMPx() : 0.0;
}

int RawFile::GetBlackLevelFromMetadata() const {
    return m_is_loaded ? m_metadata_extractor->GetBlackLevelFromMetadata() : 0;
}

int RawFile::GetActiveWidth() const {
    return m_is_loaded ? m_metadata_extractor->GetActiveWidth() : 0;
}

int RawFile::GetActiveHeight() const {
    return m_is_loaded ? m_metadata_extractor->GetActiveHeight() : 0;
}

int RawFile::GetTopMargin() const {
    return m_is_loaded ? m_metadata_extractor->GetTopMargin() : 0;
}

int RawFile::GetLeftMargin() const {
    return m_is_loaded ? m_metadata_extractor->GetLeftMargin() : 0;
}

std::optional<int> RawFile::GetBitDepth() const {
    return m_is_loaded ? m_metadata_extractor->GetBitDepth() : std::nullopt;
}

/**
 * @brief Implementación de GetOrientation.
 */
int RawFile::GetOrientation() const {
    return m_is_loaded ? m_metadata_extractor->GetOrientation() : 0;
}