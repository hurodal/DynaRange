/**
 * @file core/RawFile.cpp
 * @brief Implements the RawFile class for handling RAW image files.
 */
#include "RawFile.hpp"
#include <opencv2/imgproc.hpp>

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