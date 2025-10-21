// File: src/core/graphics/ImageProcessing.cpp
/**
 * @file core/graphics/ImageProcessing.cpp
 * @brief Implements high-level image processing orchestration and utilities.
 */
#include "ImageProcessing.hpp"
#include "../DebugConfig.hpp"
#include "geometry/KeystoneCorrection.hpp"
#include "../artifacts/ArtifactFactory.hpp"   
#include "../utils/OutputNamingContext.hpp"   
#include "../utils/OutputFilenameGenerator.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>

#define _(string) gettext(string)


namespace { // Anonymous namespace for internal helpers

void ExtractBayerChannel(const cv::Mat& src, cv::Mat& dst, DataSource channel, const std::string& pattern) {
    int r_offset = 0, c_offset = 0;

    // Determine the row/column offsets for the top-left (0,0) pixel of the 2x2 Bayer block
    if (pattern == "RGGB") {
        if (channel == DataSource::R)  { r_offset = 0; c_offset = 0; }
        if (channel == DataSource::G1) { r_offset = 0; c_offset = 1; }
        if (channel == DataSource::G2) { r_offset = 1; c_offset = 0; }
        if (channel == DataSource::B)  { r_offset = 1; c_offset = 1; }
    } else if (pattern == "GRBG") {
        if (channel == DataSource::G1) { r_offset = 0; c_offset = 0; }
        if (channel == DataSource::R)  { r_offset = 0; c_offset = 1; }
        if (channel == DataSource::B)  { r_offset = 1; c_offset = 0; }
        if (channel == DataSource::G2) { r_offset = 1; c_offset = 1; }
    } else if (pattern == "GBRG") {
        if (channel == DataSource::G1) { r_offset = 0; c_offset = 0; }
        if (channel == DataSource::B)  { r_offset = 0; c_offset = 1; }
        if (channel == DataSource::R)  { r_offset = 1; c_offset = 0; }
        if (channel == DataSource::G2) { r_offset = 1; c_offset = 1; }
    } else if (pattern == "BGGR") {
        if (channel == DataSource::B)  { r_offset = 0; c_offset = 0; }
        if (channel == DataSource::G1) { r_offset = 0; c_offset = 1; }
        if (channel == DataSource::G2) { r_offset = 1; c_offset = 0; }
        if (channel == DataSource::R)  { r_offset = 1; c_offset = 1; }
    } else { // Fallback to RGGB for unknown patterns
        if (channel == DataSource::R)  { r_offset = 0; c_offset = 0; }
        if (channel == DataSource::G1) { r_offset = 0; c_offset = 1; }
        if (channel == DataSource::G2) { r_offset = 1; c_offset = 0; }
        if (channel == DataSource::B)  { r_offset = 1; c_offset = 1; }
    }

    for (int r = 0; r < dst.rows; ++r) {
        for (int c = 0; c < dst.cols; ++c) {
            dst.at<float>(r, c) = src.at<float>(r * 2 + r_offset, c * 2 + c_offset);
        }
    }
} 

} // end anonymous namespace/ end anonymous namespace

cv::Mat NormalizeRawImage(const cv::Mat& raw_image, double black_level, double sat_level)
{
    if (raw_image.empty()) {
        return {};
    }
    cv::Mat float_img;
    raw_image.convertTo(float_img, CV_32F);

    // Normalize the image to a 0.0-1.0 range
    float_img = (float_img - black_level) / (sat_level - black_level);
    return float_img;
}

/**
 * @brief Creates the final, viewable debug image from the overlay data using the min/max visualization method.
 * @details Applies consistent visualization processing (THRESH_TOZERO, min/max normalization, gamma)
 * to the input image which already contains overlays.
 * @param overlay_image The single-channel image (CV_32F) with patch outlines drawn on it.
 * @param max_pixel_value The maximum signal value from valid patches (currently unused, kept for signature consistency).
 * @return A gamma-corrected cv::Mat (CV_32F BGR, range 0.0-1.0) ready for saving.
 */
cv::Mat CreateFinalDebugImage(const cv::Mat& overlay_image, double max_pixel_value)
{
    if (overlay_image.empty()) {
        return {}; // Return empty if input is invalid
    }

    // Apply the specific min/max + gamma visualization method
    cv::Mat final_view = ApplyMinMaxNormalizationView(overlay_image);

    // Return the resulting BGR image
    return final_view;
}

/**
 * @brief Draws cross markers on an image at specified corner locations, always in red.
 * @param image The source/destination image (CV_32FC3 BGR) to draw on. Modified in place.
 * @param corners A vector of 4 corner points in the image's coordinate system.
 * @return The input image matrix with markers drawn (returned by reference).
 */
cv::Mat& DrawCornerMarkers(cv::Mat& image, const std::vector<cv::Point2d>& corners)
{
    // Ensure the input image is 3-channel BGR (float or uchar)
    if (image.empty() || image.channels() != 3) {
         // Optionally log an error or warning
         return image; // Return unmodified image
    }

    // --- MODIFICATION START ---
    // Always use red color and debug style attributes, removing the conditional compilation.
    // Ensure the color constant DYNA_RANGE_DEBUG_CORNER_MARKER_COLOR is defined in DebugConfig.hpp.
    #if defined(DYNA_RANGE_DEBUG_CORNER_MARKER_COLOR)
        const cv::Scalar marker_color = cv::Scalar(
            DynaRange::Debug::CORNER_MARKER_COLOR[0], // B (Should be 0.0 for Red)
            DynaRange::Debug::CORNER_MARKER_COLOR[1], // G (Should be 0.0 for Red)
            DynaRange::Debug::CORNER_MARKER_COLOR[2]  // R (Should be 1.0 for Red)
        );
    #else
        // Fallback to a hardcoded bright red if the constant is not defined
        const cv::Scalar marker_color = cv::Scalar(0.0, 0.0, 1.0); // BGR for Red
    #endif

    constexpr int thickness = 2; // Always use the thicker line
    constexpr int markerSize = 40; // Always use the larger size
    // --- MODIFICATION END ---

    // Draw markers directly onto the input image
    for (const auto& point : corners) {
        // Use cv::Point for drawing functions, converting from cv::Point2d
        cv::drawMarker(image, cv::Point(cvRound(point.x), cvRound(point.y)), marker_color, cv::MARKER_CROSS, markerSize, thickness);
    }
    return image; // Return the modified image by reference
}

/**
 * @brief Prepares a single-channel float image for consistent visual debugging display.
 * @details Applies robust contrast stretching based on percentiles and gamma correction.
 * @param img_float The input single-channel image (CV_32F), assumed normalized by black/saturation.
 * @return A 3-channel BGR image (CV_32F, range [0,1]) ready for drawing overlays or saving. Returns empty Mat on error.
 */
cv::Mat PrepareDebugImageView(const cv::Mat& img_float) {
    if (img_float.empty() || img_float.type() != CV_32FC1) {
        return {};
    }

    // --- Calculate robust min/max using percentiles (e.g., 0.1% and 99.9%) ---
    cv::Mat hist;
    int histSize = 256;
    // Estimate range based on typical normalized values, adjust if needed
    float range[] = { -0.1f, 1.1f }; // Slightly wider range to catch potential undershoot/overshoot
    const float* histRange = { range };
    bool uniform = true; bool accumulate = false;
    // Calculate histogram only on valid pixels (e.g., > -infinity if any transformation caused it)
    cv::Mat valid_mask = img_float > -std::numeric_limits<float>::infinity();
    cv::calcHist(&img_float, 1, 0, valid_mask, hist, 1, &histSize, &histRange, uniform, accumulate);

    float total_valid_pixels = cv::countNonZero(valid_mask);
    if (total_valid_pixels == 0) return {}; // Handle fully masked image

    float lower_percentile_count = total_valid_pixels * 0.001f; // 0.1%
    float upper_percentile_count = total_valid_pixels * 0.999f; // 99.9%

    float robust_min = range[0]; // Initialize with range start
    float robust_max = range[1]; // Initialize with range end
    float cumulative_count = 0.0f;
    bool min_found = false;

    // Find percentile values from histogram
    for(int i = 0; i < histSize; i++) {
        float bin_value = hist.at<float>(i);
        // Check if bin_value is finite before adding
        if (!std::isfinite(bin_value)) continue;
        float current_bin_start = range[0] + (i * (range[1] - range[0]) / histSize);
        float next_bin_start = range[0] + ((i + 1) * (range[1] - range[0]) / histSize);

        if (!min_found && cumulative_count + bin_value >= lower_percentile_count) {
            // Interpolate within the bin if possible
            if (bin_value > 0) {
                 robust_min = current_bin_start + ((lower_percentile_count - cumulative_count) / bin_value) * (next_bin_start - current_bin_start);
            } else {
                 robust_min = current_bin_start; // Assign bin start if bin is empty
            }
             min_found = true;
        }
        if (cumulative_count + bin_value >= upper_percentile_count) {
             if (bin_value > 0) {
                 robust_max = current_bin_start + ((upper_percentile_count - cumulative_count) / bin_value) * (next_bin_start - current_bin_start);
             } else {
                 robust_max = current_bin_start;
             }
            break; // Found upper percentile
        }
        cumulative_count += bin_value;
    }


    // Ensure max > min and handle edge cases
    if (robust_max <= robust_min) {
         double min_val_raw, max_val_raw;
         cv::minMaxLoc(img_float, &min_val_raw, &max_val_raw, nullptr, nullptr, valid_mask);
         robust_min = static_cast<float>(min_val_raw);
         robust_max = static_cast<float>(max_val_raw);
         // If still max <= min (e.g., flat image), use default range
         if (robust_max <= robust_min) {
             robust_min = 0.0f;
             robust_max = 1.0f;
         }
    }
    // Clamp robust min/max just in case calculation went out of expected bounds
    robust_min = std::max(range[0], robust_min);
    robust_max = std::min(range[1], robust_max);


    // --- Apply manual normalization ---
    cv::Mat view_img;
    cv::subtract(img_float, cv::Scalar(robust_min), view_img);
    // Avoid division by zero or near-zero
    float range_diff = robust_max - robust_min;
    if (range_diff > 1e-6) {
        view_img /= range_diff;
    } else {
        // If range is tiny, just set to mid-gray or keep as is (potentially all zeros)
         view_img = cv::Scalar(0.5); // Or handle as appropriate
    }


    // --- Clamp [0, 1] ---
    cv::threshold(view_img, view_img, 1.0, 1.0, cv::THRESH_TRUNC);
    cv::threshold(view_img, view_img, 0.0, 0.0, cv::THRESH_TOZERO);

    // --- Apply gamma ---
    cv::Mat view_img_gamma;
    cv::pow(view_img, 1.0 / 2.2, view_img_gamma);

    // --- Convert to BGR ---
    cv::Mat final_bgr_img;
    // Ensure the gamma corrected image is finite before converting
    cv::patchNaNs(view_img_gamma, 0.0); // Replace NaNs with 0
    cv::cvtColor(view_img_gamma, final_bgr_img, cv::COLOR_GRAY2BGR);

    return final_bgr_img;
}

/**
 * @brief Prepares a single-channel Bayer image for analysis by extracting the channel,
 * normalizing its values, applying keystone correction, and cropping to the chart area.
 * Also handles saving intermediate debug images using the ApplyMinMaxNormalizationView method.
 * @param raw_file The source RawFile object, containing the image data and metadata.
 * @param dark_value The calibrated black level for normalization.
 * @param saturation_value The calibrated saturation level for normalization.
 * @param keystone_params The pre-calculated keystone transformation matrix.
 * @param chart The chart profile defining the geometry (corner points and grid size).
 * @param log_stream Stream for logging potential errors.
 * @param channel_to_extract The specific Bayer channel to extract (R, G1, G2, or B).
 * @param paths The PathManager for resolving debug output paths.
 * @param camera_model_name The camera model name (for debug filenames).
 * @param generate_full_debug Flag to enable extended debug image generation at runtime.
 * @return A fully prepared cv::Mat (CV_32FC1) for the specified channel, ready for patch analysis.
 * Returns an empty Mat on failure.
 */
cv::Mat PrepareChartImage(
    const RawFile& raw_file,
    double dark_value,
    double saturation_value,
    const cv::Mat& keystone_params,
    const ChartProfile& chart,
    std::ostream& log_stream,
    DataSource channel_to_extract,
    const PathManager& paths,
    const std::string& camera_model_name,
    bool generate_full_debug // Flag from AnalysisParameters
)
{
    // Use the active raw image area, which excludes masked pixels.
    cv::Mat raw_img = raw_file.GetActiveRawImage();
    if(raw_img.empty()){
        return {};
    }
    // Normalize based on black/saturation level (Range [0, ~1])
    cv::Mat img_float = NormalizeRawImage(raw_img, dark_value, saturation_value);
    // Extract the specific Bayer channel
    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    std::string filter_pattern = raw_file.GetFilterPattern();
    ExtractBayerChannel(img_float, imgBayer, channel_to_extract, filter_pattern);

    // Determine if any debug images need generating for this call
    #if DYNA_RANGE_DEBUG_MODE == 1
    // Generation is enabled if either compile-time OR runtime flag is on, and it's G1 channel
    bool should_generate_debug = (DynaRange::Debug::ENABLE_KEYSTONE_CROP_DEBUG || generate_full_debug) && channel_to_extract == DataSource::G1;
    #else
    // If not a debug build, only depends on the runtime flag
    bool should_generate_debug = generate_full_debug && channel_to_extract == DataSource::G1;
    #endif

    // --- DEBUG PRE-KEYSTONE (using ApplyMinMaxNormalizationView) ---
    if (should_generate_debug) {
        log_stream << "  - [DEBUG] Saving pre-keystone image..." << std::endl;
        // Prepare the base debug view using the min/max method
        cv::Mat pre_keystone_view_bgr = ApplyMinMaxNormalizationView(imgBayer);
        if (!pre_keystone_view_bgr.empty()) {
            // Draw overlays (original corners)
            const auto& corners_d = chart.GetCornerPoints();
            std::vector<cv::Point> corners_i; corners_i.reserve(corners_d.size()); for(const auto& pt : corners_d) corners_i.emplace_back(cv::Point(static_cast<int>(round(pt.x)), static_cast<int>(round(pt.y))));
            // Draw thin yellow lines connecting the source corner points
            cv::polylines(pre_keystone_view_bgr, corners_i, true, cv::Scalar(0, 1.0, 1.0), 1); // BGR Yellow, thin line

            // Save using ArtifactFactory
            OutputNamingContext naming_ctx_debug;
            naming_ctx_debug.camera_name_exif = camera_model_name;
            naming_ctx_debug.effective_camera_name_for_output = camera_model_name; // Use EXIF for debug img name
            fs::path debug_filename = OutputFilenameGenerator::GeneratePreKeystoneDebugFilename(naming_ctx_debug);
            ArtifactFactory::CreateGenericDebugImage(pre_keystone_view_bgr, debug_filename, paths, log_stream);
        } else {
             log_stream << "  - [DEBUG] Warning: Failed to prepare pre-keystone debug view." << std::endl;
        }
    }

    // Apply Keystone correction (geometric transformation)
    cv::Mat img_corrected = DynaRange::Graphics::Geometry::UndoKeystone(imgBayer, keystone_params);

    // Get destination points and calculate crop area
    const auto& dst_pts = chart.GetDestinationPoints();
    double xtl = dst_pts[0].x; double ytl = dst_pts[0].y;
    double xbr = dst_pts[2].x; double ybr = dst_pts[2].y;
    double gap_x = 0.0;
    double gap_y = 0.0;
    // Apply gap only if corners were auto-detected or default (not manually specified)
    if (!chart.HasManualCoords()) {
        gap_x = (xbr - xtl) / (chart.GetGridCols() + 1) / 2.0;
        gap_y = (ybr - ytl) / (chart.GetGridRows() + 1) / 2.0;
    }
    // Ensure crop dimensions are positive before creating Rect
    double crop_width = round((xbr - gap_x) - (xtl + gap_x));
    double crop_height = round((ybr - gap_y) - (ytl + gap_y));
    if (crop_width <= 0 || crop_height <= 0) {
        log_stream << _("Error: Invalid crop area dimensions calculated after keystone correction (width or height <= 0).") << std::endl;
        return {}; // Return empty matrix
    }
    cv::Rect crop_area(round(xtl + gap_x), round(ytl + gap_y), crop_width, crop_height);


    // --- DEBUG POST-KEYSTONE AND CROP AREA (using ApplyMinMaxNormalizationView) ---
    if (should_generate_debug) {
        log_stream << "  - [DEBUG] Saving post-keystone and crop area images..." << std::endl;
        // Prepare the base debug view for the corrected image using the min/max method
        cv::Mat post_keystone_base_bgr = ApplyMinMaxNormalizationView(img_corrected);
        if (!post_keystone_base_bgr.empty()) {
            // Create copies for drawing different overlays
            cv::Mat post_keystone_view_bgr = post_keystone_base_bgr.clone();
            cv::Mat crop_area_view_bgr = post_keystone_base_bgr.clone();

            // Draw destination rectangle (bounding box based on dst_pts) on post_keystone_view_bgr
            cv::rectangle(post_keystone_view_bgr,
                          cv::Point(static_cast<int>(round(xtl)), static_cast<int>(round(ytl))),
                          cv::Point(static_cast<int>(round(xbr)), static_cast<int>(round(ybr))),
                          cv::Scalar(0, 0, 1.0), 1); // BGR Red, thin line

            // Draw crop area rectangle on crop_area_view_bgr
            cv::rectangle(crop_area_view_bgr, crop_area.tl(), crop_area.br(), cv::Scalar(1.0, 0, 1.0), 1); // BGR Magenta, thin line

            // Save both images using ArtifactFactory
            OutputNamingContext naming_ctx_debug;
            naming_ctx_debug.camera_name_exif = camera_model_name;
            naming_ctx_debug.effective_camera_name_for_output = camera_model_name; // Use EXIF for debug img name
            fs::path post_filename = OutputFilenameGenerator::GeneratePostKeystoneDebugFilename(naming_ctx_debug);
            ArtifactFactory::CreateGenericDebugImage(post_keystone_view_bgr, post_filename, paths, log_stream);
            fs::path crop_filename = OutputFilenameGenerator::GenerateCropAreaDebugFilename(naming_ctx_debug);
            ArtifactFactory::CreateGenericDebugImage(crop_area_view_bgr, crop_filename, paths, log_stream);
        } else {
             log_stream << "  - [DEBUG] Warning: Failed to prepare post-keystone debug view." << std::endl;
        }
    }

    // Validate crop area boundaries against the *corrected* image dimensions
    if (crop_area.x < 0 || crop_area.y < 0 ||
        crop_area.x + crop_area.width > img_corrected.cols ||
        crop_area.y + crop_area.height > img_corrected.rows) {
        log_stream << _("Error: Invalid crop area calculated after keystone correction.")
                   << " Area: [" << crop_area.x << "," << crop_area.y << " - " << crop_area.width << "x" << crop_area.height << "]"
                   << " Image: " << img_corrected.cols << "x" << img_corrected.rows << std::endl;
        return {}; // Return empty matrix
    }

    // Return the final cropped image (img_corrected, *not* processed for viewing) for analysis
    // Ensure clone is used if the ROI might be invalidated later
    return img_corrected(crop_area).clone();
}

/**
 * @brief Applies a min/max normalization followed by gamma correction for visualization.
 * @details This method replicates the visual processing originally used for the corner detection debug image.
 * It applies THRESH_TOZERO, then normalizes using absolute min/max values (NORM_MINMAX), and finally applies gamma correction.
 * @param img_float The input single-channel image (CV_32FC1), typically normalized by black/saturation beforehand.
 * @return A 3-channel BGR image (CV_32F, range [0,1]) ready for drawing overlays or saving. Returns empty Mat on error.
 */
cv::Mat ApplyMinMaxNormalizationView(const cv::Mat& img_float) {
    if (img_float.empty() || img_float.type() != CV_32FC1) {
        return {}; // Return empty if input is invalid
    }

    // Create a copy to avoid modifying the input if it's used elsewhere
    cv::Mat processed_img = img_float.clone();

    // Apply THRESH_TOZERO first, consistent with original corner detection path
    // This removes negative values before min/max normalization
    cv::threshold(processed_img, processed_img, 0.0, 1.0, cv::THRESH_TOZERO);

    // Normalize using absolute min/max for high contrast
    // NORM_MINMAX stretches the range [min_val, max_val] to [0.0, 1.0]
    cv::normalize(processed_img, processed_img, 0.0, 1.0, cv::NORM_MINMAX);

    // Apply gamma correction
    // Input to pow should now be non-negative due to THRESH_TOZERO and NORM_MINMAX
    cv::Mat gamma_corrected_img;
    cv::pow(processed_img, 1.0 / 2.2, gamma_corrected_img);

    // Convert to 3-channel BGR for consistency with other debug outputs
    cv::Mat final_bgr_img;
    // Replace potential NaNs (though less likely now)
    cv::patchNaNs(gamma_corrected_img, 0.0);
    cv::cvtColor(gamma_corrected_img, final_bgr_img, cv::COLOR_GRAY2BGR);

    return final_bgr_img;
}