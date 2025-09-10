// File: core/RawFile.hpp
#pragma once

#include <string>
#include <libraw/libraw.h>
#include <opencv2/core/mat.hpp>

// This class has the single responsibility of loading a RAW file
// and providing access to its data and metadata.
class RawFile {
public:
    // Constructor that takes the path to the file.
    explicit RawFile(std::string filename);
    ~RawFile();

    // Loads the RAW file into memory. Returns false on failure.
    bool Load();

    // Provides a normalized, floating-point image for calculations.
    cv::Mat GetNormalizedImage(double black_level, double sat_level) const;

    // Provides direct access to the raw 16-bit sensor data.
    cv::Mat GetRawImage() const;
    
    // --- Metadata Getters ---
    std::string GetCameraModel() const;
    int GetWidth() const;
    int GetHeight() const;
    const std::string& GetFilename() const;
    bool IsLoaded() const;

private:
    std::string m_filename;
    bool m_is_loaded = false;
    
    // LibRaw data structures
    LibRaw m_raw_processor;
    libraw_processed_image_t* m_decoded_image = nullptr;

    // Cached data
    mutable cv::Mat m_raw_image_cache;
    mutable std::string m_camera_model_cache;
};