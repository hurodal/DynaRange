// File: src/core/io/raw/RawImageAccessor.hpp
/**
 * @file RawImageAccessor.hpp
 * @brief Declares a component for accessing image data from a loaded RAW file.
 * @details This module adheres to SRP by handling the extraction and conversion
 * of pixel data (raw, active area, processed RGB) from a LibRaw object.
 */
#pragma once

#include <libraw/libraw.h>
#include <opencv2/core/mat.hpp>
#include <memory>

namespace DynaRange::IO::Raw {

/**
 * @class RawImageAccessor
 * @brief Provides access to various image data representations from a LibRaw object.
 */
class RawImageAccessor {
public:
    explicit RawImageAccessor(std::shared_ptr<LibRaw> raw_processor);

    cv::Mat GetRawImage() const;
    cv::Mat GetActiveRawImage() const;
    cv::Mat GetProcessedImage();

private:
    std::shared_ptr<LibRaw> m_raw_processor;
    mutable cv::Mat m_raw_image_cache;
    mutable cv::Mat m_active_raw_image_cache;
};

} // namespace DynaRange::IO::Raw