// File: src/core/io/RawFile.cpp
/**
 * @file src/core/io/RawFile.cpp
 * @brief Implements the RawFile class for handling RAW image files.
 */
#include "RawFile.hpp"
#include <opencv2/imgproc.hpp>
#include <cmath>

RawFile::RawFile(std::string filename) : m_filename(std::move(filename)) {}

RawFile::~RawFile() {
    if (m_decoded_image) {
        LibRaw::dcraw_clear_mem(m_decoded_image);
    }
}

bool RawFile::Load() {
    if (m_is_loaded) return true;

    if (m_raw_processor.open_file(m_filename.c_str()) != LIBRAW_SUCCESS) {
        return false;
    }
    if (m_raw_processor.unpack() != LIBRAW_SUCCESS) {
        return false;
    }

    m_is_loaded = true;
    return true;
}

cv::Mat RawFile::GetRawImage() const {
    if (!m_is_loaded) return {};
    if (!m_raw_image_cache.empty()) return m_raw_image_cache;

    m_raw_image_cache = cv::Mat(
        m_raw_processor.imgdata.sizes.raw_height,
        m_raw_processor.imgdata.sizes.raw_width,
        CV_16U,
        m_raw_processor.imgdata.rawdata.raw_image
    );
    return m_raw_image_cache;
}

cv::Mat RawFile::GetNormalizedImage(double black_level, double sat_level) const {
    if (!m_is_loaded) return {};

    cv::Mat raw_img = GetRawImage();
    cv::Mat float_img;
    raw_img.convertTo(float_img, CV_32F);

    // Normalize the image to a 0.0-1.0 range
    float_img = (float_img - black_level) / (sat_level - black_level);
    return float_img;
}

std::string RawFile::GetCameraModel() const {
    if (!m_is_loaded) return "";
    if (!m_camera_model_cache.empty()) return m_camera_model_cache;

    m_camera_model_cache = std::string(m_raw_processor.imgdata.idata.model);
    return m_camera_model_cache;
}

float RawFile::GetIsoSpeed() const {
    if (!m_is_loaded) return 0.0f;
    if (m_iso_speed_cache > 0.0f) {
        return m_iso_speed_cache; // Use cached value
    }

    m_iso_speed_cache = m_raw_processor.imgdata.other.iso_speed;
    return m_iso_speed_cache;
}

int RawFile::GetWidth() const {
    return m_is_loaded ? m_raw_processor.imgdata.sizes.raw_width : 0;
}

int RawFile::GetHeight() const {
    return m_is_loaded ? m_raw_processor.imgdata.sizes.raw_height : 0;
}

const std::string& RawFile::GetFilename() const {
    return m_filename;
}

bool RawFile::IsLoaded() const {
    return m_is_loaded;
}

/**
 * @brief Gets the sensor resolution in megapixels from the RAW file's metadata.
 * @return The sensor resolution in Mpx (e.g., 16.0 for Olympus OM-1). 
 *         Returns 0.0 if unavailable or invalid.
 */
double RawFile::GetSensorResolutionMPx() const {
    if (!m_is_loaded) return 0.0;

    int width = m_raw_processor.imgdata.sizes.raw_width;
    int height = m_raw_processor.imgdata.sizes.raw_height;

    if (width <= 0 || height <= 0) return 0.0;

    // Calculate total pixels in megapixels
    double total_pixels = static_cast<double>(width) * height;
    return total_pixels / 1000000.0; // Convert to Mpx
}