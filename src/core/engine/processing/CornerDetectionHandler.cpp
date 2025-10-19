// File: src/core/engine/processing/CornerDetectionHandler.cpp
/**
 * @file CornerDetectionHandler.cpp
 * @brief Implements the automatic chart corner detection logic.
 */
#include "CornerDetectionHandler.hpp"
#include "../../artifacts/ArtifactFactory.hpp"
#include "../../graphics/ImageProcessing.hpp"
#include "../../graphics/detection/ChartCornerDetector.hpp"
#include "../../io/OutputWriter.hpp"
#include "../../utils/OutputNamingContext.hpp"
#include "../../utils/OutputFilenameGenerator.hpp"
#include "../../DebugConfig.hpp" // Check path if needed
#include "../Constants.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>
#include <optional>
#include <ostream>
#include <filesystem>
#include <iomanip>
#include <opencv2/core.hpp> // For cv::Point2f, cv::contourArea

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace DynaRange::Engine::Processing {

/**
 * @brief Attempts to automatically detect chart corners if no manual coordinates are provided.
 * @details Extracts G1 channel, detects corners, optionally saves debug image using ArtifactFactory, // Updated Doxygen
 * and validates detected area.
 * @param source_raw_file The single loaded RAW file to use for detection.
 * @param chart_coords The vector of manually provided chart coordinates.
 * @param dark_value The black level for normalization.
 * @param saturation_value The saturation level for normalization.
 * @param paths The PathManager for resolving debug output paths.
 * @param log_stream The output stream for logging.
 * @return An optional containing a vector of 4 corner points on success, or std::nullopt on failure or if not needed.
 */
std::optional<std::vector<cv::Point2d>> AttemptAutomaticCornerDetection(
    const RawFile& source_raw_file,
    const std::vector<double>& chart_coords,
    double dark_value,
    double saturation_value,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // Return immediately if manual coordinates are provided or the file isn't loaded
    if (!chart_coords.empty() || !source_raw_file.IsLoaded()) {
        return std::nullopt;
    }

    log_stream << _("Manual coordinates not provided, attempting automatic corner detection...") << std::endl;
    cv::Mat raw_img = source_raw_file.GetActiveRawImage();
    if (raw_img.empty()) {
         log_stream << _("Error: Could not get active raw image for corner detection.") << std::endl;
         return std::nullopt;
    }

    // Normalize the raw image
    cv::Mat img_float = NormalizeRawImage(raw_img, dark_value, saturation_value);
    if (img_float.empty()) {
        log_stream << _("Error: Normalization failed during corner detection.") << std::endl;
        return std::nullopt;
    }

    // Extract the G1 Bayer channel (assuming G1 is needed for corner markers)
    int bayer_rows = img_float.rows / 2;
    int bayer_cols = img_float.cols / 2;
    cv::Mat g1_bayer = cv::Mat::zeros(bayer_rows, bayer_cols, CV_32FC1);
    std::string pattern = source_raw_file.GetFilterPattern();
    // Calculate offsets based on pattern (simplified example for G1)
    int r_offset = 0, c_offset = 1; // Default for RGGB
    if (pattern == "GRBG" || pattern == "GBRG") { r_offset = 0; c_offset = 0; }
    else if (pattern == "BGGR") { r_offset = 1; c_offset = 1; }
    // Add other patterns if necessary

    for (int r = 0; r < bayer_rows; ++r) {
        for (int c = 0; c < bayer_cols; ++c) {
            int src_r = r * 2 + r_offset;
            int src_c = c * 2 + c_offset;
            if (src_r >= 0 && src_r < img_float.rows && src_c >= 0 && src_c < img_float.cols) {
                 g1_bayer.at<float>(r, c) = img_float.at<float>(src_r, src_c);
            }
        }
    }
    // Apply threshold
    cv::threshold(g1_bayer, g1_bayer, 0.0, 1.0, cv::THRESH_TOZERO);
    // Perform corner detection
    std::optional<std::vector<cv::Point2d>> detected_corners_opt = DynaRange::Graphics::Detection::DetectChartCorners(g1_bayer, log_stream);

    // Save debug image if debug mode is enabled and corners were found
    #if DYNA_RANGE_DEBUG_MODE == 1
    if (DynaRange::Debug::ENABLE_CORNER_DETECTION_DEBUG && detected_corners_opt.has_value()) {
        log_stream << "  - [DEBUG] Saving corner detection visual confirmation..." << std::endl;
        cv::Mat viewable_image;
        cv::normalize(g1_bayer, viewable_image, 0.0, 1.0, cv::NORM_MINMAX);
        cv::Mat image_with_markers = DrawCornerMarkers(viewable_image, *detected_corners_opt);
        cv::Mat final_debug_image;
        cv::pow(image_with_markers, 1.0 / 2.2, final_debug_image);

        // Create context for Factory
        OutputNamingContext naming_ctx_corner;
        naming_ctx_corner.camera_name_exif = source_raw_file.GetCameraModel();
        naming_ctx_corner.effective_camera_name_for_output = ""; // Debug context: no suffix

        // Use ArtifactFactory to create and save the debug image
        std::optional<fs::path> saved_path = ArtifactFactory::CreateCornerDebugImage(
            final_debug_image,
            naming_ctx_corner,
            paths, // PathManager is already passed to this function
            log_stream
        );
        // Logging is handled by the factory, but we can add a warning on failure
        if (!saved_path) {
            log_stream << _("Warning: Failed to save corner detection debug image.") << std::endl;
        }

    }
    #endif

    // Validate the area of the detected chart polygon
    if (detected_corners_opt.has_value()) {
        std::vector<cv::Point2f> corners_float;
        for (const auto& pt : *detected_corners_opt) {
            corners_float.push_back(cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y)));
        }
        double total_image_area = static_cast<double>(g1_bayer.cols * g1_bayer.rows);
        double detected_chart_area = cv::contourArea(corners_float);
        double area_percentage = (total_image_area > 0) ? (detected_chart_area / total_image_area) : 0.0;

        if (area_percentage < DynaRange::Engine::Constants::MINIMUM_CHART_AREA_PERCENTAGE) {
            log_stream << _("Warning: Automatic corner detection found an area covering only ")
                       << std::fixed << std::setprecision(1) << (area_percentage * 100.0)
                       << _("% of the image. This is below the required threshold of ")
                       << (DynaRange::Engine::Constants::MINIMUM_CHART_AREA_PERCENTAGE * 100.0)
                       << _(" %. Discarding detected corners and falling back to defaults.") << std::endl;
            detected_corners_opt.reset();
        }
    }

    return detected_corners_opt;
}

} // namespace DynaRange::Engine::Processing