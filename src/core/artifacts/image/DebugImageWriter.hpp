// File: src/core/artifacts/image/DebugImageWriter.hpp
/**
 * @file src/core/artifacts/image/DebugImageWriter.hpp
 * @brief Declares functions for creating and saving debug/auxiliary image artifacts.
 * @details This module centralizes all logic for naming and saving non-plot and non-chart debug images,
 * adhering to the Single Responsibility Principle (SRP) by managing pixel image output.
 */
#pragma once

#include "../../utils/OutputNamingContext.hpp"
#include "../../utils/PathManager.hpp"
#include <opencv2/core.hpp>
#include <filesystem>
#include <optional>
#include <ostream>

namespace fs = std::filesystem;

namespace ArtifactFactory::Image { 

/**
 * @enum DebugImageType
 * @brief Defines the specific type of debug image to be generated and named for CreateGenericDebugImage.
 */
enum class DebugImageType { 
    PreKeystone, 
    PostKeystone, 
    CropArea,
    Corners
};

/**
 * @brief Creates and saves the debug image showing analyzed patches (for -g/--print-patches).
 * @param debug_image The OpenCV matrix (CV_32F, 0-1 range, gamma corrected) to save.
 * @param ctx The context for generating the filename.
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved image file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreatePrintPatchesImage(
    const cv::Mat& debug_image,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream);

/**
 * @brief Creates and saves a generic debug image (e.g., pre/post keystone, crop area).
 * @param debug_image The OpenCV matrix (CV_32F, 0-1 range, gamma corrected) with markers/overlays.
 * @param ctx The context for generating the filename.
 * @param debug_type The specific type of image to name and save (e.g., PreKeystone, CropArea).
 * @param paths The PathManager to resolve the final output path.
 * @param log_stream Stream for logging messages.
 * @return An optional containing the full path to the saved image file on success, or std::nullopt on failure.
 */
std::optional<fs::path> CreateGenericDebugImage(
    const cv::Mat& debug_image,
    const OutputNamingContext& ctx,
    DebugImageType debug_type,
    const PathManager& paths,
    std::ostream& log_stream);

} // namespace ArtifactFactory::Image