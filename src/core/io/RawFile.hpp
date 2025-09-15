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
 * @details This class acts as a wrapper around the LibRaw library to safely
 * load a RAW file, cache its data, and provide access to image data
 * (in raw or normalized formats) and essential metadata like the camera model.
 */
class RawFile {
public:
    /**
     * @brief Constructs a RawFile object.
     * @param filename The path to the RAW file.
     */
    explicit RawFile(std::string filename);

    /**
     * @brief Destructor to clean up LibRaw resources.
     */
    ~RawFile();

    /**
     * @brief Loads the RAW file's metadata and unpacks the image data.
     * @details This must be called before any data can be retrieved.
     * It is safe to call multiple times.
     * @return true if loading was successful, false otherwise.
     */
    bool Load();

    /**
     * @brief Gets the image data normalized to a [0.0, 1.0] float range.
     * @param black_level The black level to subtract from the raw data.
     * @param sat_level The saturation level to use for normalization.
     * @return A 32-bit floating-point cv::Mat. Returns an empty Mat on failure.
     */
    cv::Mat GetNormalizedImage(double black_level, double sat_level) const;

    /**
     * @brief Gets direct access to the raw 16-bit sensor data.
     * @details The data is cached internally after the first call.
     * @return A 16-bit unsigned cv::Mat. Returns an empty Mat on failure.
     */
    cv::Mat GetRawImage() const;

    // --- Metadata Getters ---

    /**
     * @brief Gets the camera model name from the file's metadata.
     * @return The camera model as a std::string.
     */
    std::string GetCameraModel() const;

    /**
     * @brief Gets the raw width of the image.
     * @return The width in pixels.
     */
    int GetWidth() const;

    /**
     * @brief Gets the raw height of the image.
     * @return The height in pixels.
     */
    int GetHeight() const;

    /**
     * @brief Gets the filename of the RAW file.
     * @return A const reference to the filename.
     */
    const std::string& GetFilename() const;

    /**
     * @brief Checks if the file has been successfully loaded.
     * @return true if loaded, false otherwise.
     */
    bool IsLoaded() const;

    /**
     * @brief Gets the ISO speed from the file's metadata.
     * @return The ISO speed as a float. Returns 0.0 if not available.
     */
    float GetIsoSpeed() const;

private:
    std::string m_filename;      ///< Path to the RAW file.
    bool m_is_loaded = false;    ///< Flag indicating if Load() was successful.

    // --- LibRaw data structures ---
    LibRaw m_raw_processor;      ///< The LibRaw processing object.
    libraw_processed_image_t* m_decoded_image = nullptr; ///< Pointer for decoded image data (if used).

    // --- Cached data ---
    mutable cv::Mat m_raw_image_cache; ///< Cached raw image Mat to avoid reprocessing.
    mutable std::string m_camera_model_cache; ///< Cached camera model string.
    mutable float m_iso_speed_cache = 0.0f; ///< Cached ISO speed value.
};