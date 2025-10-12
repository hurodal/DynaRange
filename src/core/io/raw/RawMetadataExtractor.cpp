// File: src/core/io/raw/RawMetadataExtractor.cpp
/**
 * @file RawMetadataExtractor.cpp
 * @brief Implements the metadata extraction component for RAW files.
 */
#include "RawMetadataExtractor.hpp"
#include <cmath>

namespace DynaRange::IO::Raw {

RawMetadataExtractor::RawMetadataExtractor(std::shared_ptr<LibRaw> raw_processor)
    : m_raw_processor(raw_processor) {}

std::string RawMetadataExtractor::GetCameraModel() const {
    if (!m_raw_processor) return "";
    if (!m_camera_model_cache.empty()) return m_camera_model_cache;
    m_camera_model_cache = std::string(m_raw_processor->imgdata.idata.model);
    return m_camera_model_cache;
}

float RawMetadataExtractor::GetIsoSpeed() const {
    if (!m_raw_processor) return 0.0f;
    if (m_iso_speed_cache > 0.0f) {
        return m_iso_speed_cache;
    }
    m_iso_speed_cache = m_raw_processor->imgdata.other.iso_speed;
    return m_iso_speed_cache;
}

int RawMetadataExtractor::GetWidth() const {
    return m_raw_processor ? m_raw_processor->imgdata.sizes.raw_width : 0;
}

int RawMetadataExtractor::GetHeight() const {
    return m_raw_processor ? m_raw_processor->imgdata.sizes.raw_height : 0;
}

double RawMetadataExtractor::GetSensorResolutionMPx() const {
    if (!m_raw_processor) return 0.0;
    int width = m_raw_processor->imgdata.sizes.raw_width;
    int height = m_raw_processor->imgdata.sizes.raw_height;
    if (width <= 0 || height <= 0) return 0.0;
    double total_pixels = static_cast<double>(width) * height;
    return total_pixels / 1000000.0;
}

int RawMetadataExtractor::GetBlackLevelFromMetadata() const {
    if (!m_raw_processor) return 0;
    int black_level = m_raw_processor->imgdata.color.black;
    if (black_level > 0) {
        return black_level;
    }
    double sum = 0;
    int count = 0;
    for (int i = 0; i < LIBRAW_CBLACK_SIZE; ++i) {
        if (m_raw_processor->imgdata.color.cblack[i] > 0) {
            sum += m_raw_processor->imgdata.color.cblack[i];
            count++;
        }
    }
    if (count > 0) {
        return static_cast<int>(std::round(sum / count));
    }
    return 0;
}

int RawMetadataExtractor::GetActiveWidth() const {
    return m_raw_processor ? m_raw_processor->imgdata.sizes.width : 0;
}

int RawMetadataExtractor::GetActiveHeight() const {
    return m_raw_processor ? m_raw_processor->imgdata.sizes.height : 0;
}

int RawMetadataExtractor::GetTopMargin() const {
    return m_raw_processor ? m_raw_processor->imgdata.sizes.top_margin : 0;
}

int RawMetadataExtractor::GetLeftMargin() const {
    return m_raw_processor ? m_raw_processor->imgdata.sizes.left_margin : 0;
}

std::optional<int> RawMetadataExtractor::GetBitDepth() const {
    if (!m_raw_processor) return std::nullopt;
    int max_val = m_raw_processor->imgdata.color.maximum;
    if (max_val > 0) {
        return static_cast<int>(std::ceil(std::log2(static_cast<double>(max_val))));
    }
    return std::nullopt;
}

} // namespace DynaRange::IO::Raw