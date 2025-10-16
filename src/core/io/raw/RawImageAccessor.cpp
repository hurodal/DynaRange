// File: src/core/io/raw/RawImageAccessor.cpp
/**
 * @file RawImageAccessor.cpp
 * @brief Implements the image data accessor for RAW files.
 */
#include "RawImageAccessor.hpp"
#include <opencv2/imgproc.hpp>

namespace DynaRange::IO::Raw {

RawImageAccessor::RawImageAccessor(std::shared_ptr<LibRaw> raw_processor)
    : m_raw_processor(raw_processor) {}

cv::Mat RawImageAccessor::GetRawImage() const {
    if (!m_raw_processor) return {};
    if (!m_raw_image_cache.empty()) return m_raw_image_cache;

    if (!m_raw_processor->imgdata.rawdata.raw_image) {
        return {};
    }

    m_raw_image_cache = cv::Mat(
        m_raw_processor->imgdata.sizes.raw_height,
        m_raw_processor->imgdata.sizes.raw_width,
        CV_16U,
        m_raw_processor->imgdata.rawdata.raw_image
    );
    return m_raw_image_cache;
}

cv::Mat RawImageAccessor::GetActiveRawImage() const {
    if (!m_raw_processor) return {};
    if (!m_active_raw_image_cache.empty()) return m_active_raw_image_cache;

    cv::Mat full_raw_image = GetRawImage();
    if (full_raw_image.empty()) return {};

    cv::Rect active_area(
        m_raw_processor->imgdata.sizes.left_margin,
        m_raw_processor->imgdata.sizes.top_margin,
        m_raw_processor->imgdata.sizes.width,
        m_raw_processor->imgdata.sizes.height
    );

    if (active_area.width <= 0 || active_area.height <= 0 ||
        (active_area.x + active_area.width) > full_raw_image.cols ||
        (active_area.y + active_area.height) > full_raw_image.rows) {
        m_active_raw_image_cache = full_raw_image.clone();
        return m_active_raw_image_cache;
    }

    m_active_raw_image_cache = full_raw_image(active_area).clone();
    return m_active_raw_image_cache;
}

cv::Mat RawImageAccessor::GetProcessedImage() {
    if (!m_raw_processor) return {};

    // FORCED: Disable LibRaw's automatic rotation based on EXIF data.
    m_raw_processor->imgdata.params.user_flip = 0;

    if (m_raw_processor->dcraw_process() != LIBRAW_SUCCESS) {
        return {};
    }

    libraw_processed_image_t* processed_image = m_raw_processor->dcraw_make_mem_image();
    if (processed_image == nullptr || processed_image->type != LIBRAW_IMAGE_BITMAP || processed_image->bits != 8 || processed_image->colors != 3) {
        if (processed_image) LibRaw::dcraw_clear_mem(processed_image);
        return {};
    }

    cv::Mat image_mat(processed_image->height, processed_image->width, CV_8UC3, processed_image->data);
    cv::Mat bgr_image;
    cv::cvtColor(image_mat, bgr_image, cv::COLOR_RGB2BGR);
    LibRaw::dcraw_clear_mem(processed_image);

    return bgr_image;
}

} // namespace DynaRange::IO::Raw