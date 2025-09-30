// File: src/core/io/RawFile.hpp
/**
 * @file src/core/io/RawFile.hpp
 * @brief Defines the RawFile class for handling RAW image files.
 */
#pragma once

#include <string>
#include <libraw/libraw.h>
#include <opencv2/core/mat.hpp>

/**
 * @class RawFile
 * @brief Manages the loading and data access of a single RAW image file.
 */
class RawFile {
public:
    explicit RawFile(std::string filename);
    ~RawFile();
    bool Load();
    /**
     * @brief Gets direct access to the raw 16-bit sensor data.
     * @return A 16-bit unsigned cv::Mat. Returns an empty Mat on failure.
     */
    cv::Mat GetRawImage() const;
    /**
     * @brief Gets a cv::Mat containing only the active (non-masked) area of the raw sensor data.
     * @return A 16-bit unsigned, continuous cv::Mat. Returns an empty Mat on failure.
     */
    cv::Mat GetActiveRawImage() const;
    /**
     * @brief Gets a processed, 8-bit, 3-channel sRGB image.
     * @details This performs a full demosaic and color space conversion using
     * LibRaw's internal pipeline to produce a standard viewable image.
     * @return A CV_8UC3 cv::Mat in BGR order, ready for display or saving.
     */
    cv::Mat GetProcessedImage();
    
    // --- Metadata Getters ---
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

private:
    std::string m_filename;
    bool m_is_loaded = false;
    mutable LibRaw m_raw_processor;
    libraw_processed_image_t* m_decoded_image = nullptr;
    mutable cv::Mat m_raw_image_cache;
    mutable cv::Mat m_active_raw_image_cache; // Caché para la imagen activa
    mutable std::string m_camera_model_cache;
    mutable float m_iso_speed_cache = 0.0f;
};