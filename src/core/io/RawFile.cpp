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

double RawFile::GetSensorResolutionMPx() const {
    if (!m_is_loaded) return 0.0;

    int width = m_raw_processor.imgdata.sizes.raw_width;
    int height = m_raw_processor.imgdata.sizes.raw_height;
    if (width <= 0 || height <= 0) return 0.0;
    
    double total_pixels = static_cast<double>(width) * height;
    return total_pixels / 1000000.0; // Convert to Mpx
}

cv::Mat RawFile::GetProcessedImage() {
    if (!m_is_loaded) return {};
    // Use LibRaw to process the image with default settings (demosaic, color space, etc.)
    if (m_raw_processor.dcraw_process() != LIBRAW_SUCCESS) {
        return {};
    }

    libraw_processed_image_t* processed_image = m_raw_processor.dcraw_make_mem_image();
    if (processed_image == nullptr || processed_image->type != LIBRAW_IMAGE_BITMAP || processed_image->bits != 8 || processed_image->colors != 3) {
        if (processed_image) LibRaw::dcraw_clear_mem(processed_image);
        return {};
    }

    // Create an OpenCV Mat from the LibRaw data (which is RGB)
    cv::Mat image_mat(processed_image->height, processed_image->width, CV_8UC3, processed_image->data);
    
    // OpenCV uses BGR order, so we need to convert the color space.
    cv::Mat bgr_image;
    cv::cvtColor(image_mat, bgr_image, cv::COLOR_RGB2BGR);
    
    // LibRaw allocated this memory, so we must free it.
    LibRaw::dcraw_clear_mem(processed_image);

    return bgr_image;
}

int RawFile::GetBlackLevelFromMetadata() const {
    if (!m_is_loaded) return 0;
    return m_raw_processor.imgdata.color.black;
}

int RawFile::GetActiveWidth() const {
    return m_is_loaded ? m_raw_processor.imgdata.sizes.width : 0;
}

int RawFile::GetActiveHeight() const {
    return m_is_loaded ? m_raw_processor.imgdata.sizes.height : 0;
}

int RawFile::GetTopMargin() const {
    return m_is_loaded ? m_raw_processor.imgdata.sizes.top_margin : 0;
}

int RawFile::GetLeftMargin() const {
    return m_is_loaded ? m_raw_processor.imgdata.sizes.left_margin : 0;
}

cv::Mat RawFile::GetActiveRawImage() const {
    if (!m_is_loaded) return {};
    // If the active image is already in the cache, return it directly.
    if (!m_active_raw_image_cache.empty()) return m_active_raw_image_cache;

    // Get the full raw image.
    cv::Mat full_raw_image = GetRawImage();
    if (full_raw_image.empty()) return {};

    // Define the active area using the getters that already exist.
    cv::Rect active_area(
        GetLeftMargin(),
        GetTopMargin(),
        GetActiveWidth(),
        GetActiveHeight()
    );

    // Check the validity of the area.
    if (active_area.width <= 0 || active_area.height <= 0 ||
        (active_area.x + active_area.width) > full_raw_image.cols ||
        (active_area.y + active_area.height) > full_raw_image.rows) {
        // If the area is not valid, return the full image as a fallback.
        m_active_raw_image_cache = full_raw_image.clone();
        return m_active_raw_image_cache;
    }

    // Crop, clone (to ensure continuous memory), and cache.
    m_active_raw_image_cache = full_raw_image(active_area).clone();
    return m_active_raw_image_cache;
}