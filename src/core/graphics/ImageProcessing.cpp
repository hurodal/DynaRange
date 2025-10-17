// File: src/core/graphics/ImageProcessing.cpp
/**
 * @file core/graphics/ImageProcessing.cpp
 * @brief Implements high-level image processing orchestration and utilities.
 */
#include "ImageProcessing.hpp"
#include "geometry/KeystoneCorrection.hpp"
#include "../DebugConfig.hpp"
#include "../io/raw/RawFile.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

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

} // end anonymous namespace
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

cv::Mat PrepareChartImage(
    const RawFile& raw_file, 
    double dark_value,
    double saturation_value,
    const cv::Mat& keystone_params,
    const ChartProfile& chart, 
    std::ostream& log_stream,
    DataSource channel_to_extract,
    const PathManager& paths)
{
    // Use the active raw image area, which excludes masked pixels.
    cv::Mat raw_img = raw_file.GetActiveRawImage();
    if(raw_img.empty()){
        return {};
    }
    cv::Mat img_float = NormalizeRawImage(raw_img, dark_value, saturation_value);
    
    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    
    std::string filter_pattern = raw_file.GetFilterPattern();
    if (filter_pattern != "RGGB" && channel_to_extract == DataSource::G1) {
        log_stream << "[INFO] Detected non-RGGB Bayer pattern: " << filter_pattern << ". Adjusting channel extraction." << std::endl;
    }
    ExtractBayerChannel(img_float, imgBayer, channel_to_extract, filter_pattern);
    
    cv::Mat img_corrected = DynaRange::Graphics::Geometry::UndoKeystone(imgBayer, keystone_params);

    const auto& dst_pts = chart.GetDestinationPoints();
    double xtl = dst_pts[0].x;
    double ytl = dst_pts[0].y;
    double xbr = dst_pts[2].x;
    double ybr = dst_pts[2].y;

    double gap_x = 0.0;
    double gap_y = 0.0;

    if (!chart.HasManualCoords()) {
        gap_x = (xbr - xtl) / (chart.GetGridCols() + 1) / 2.0;
        gap_y = (ybr - ytl) / (chart.GetGridRows() + 1) / 2.0;
    }

    cv::Rect crop_area(round(xtl + gap_x), round(ytl + gap_y), round((xbr - gap_x) - (xtl + gap_x)), round((ybr - gap_y) - (ytl + gap_y)));
    
    // --- VISUAL DEBUG BLOCK ---
    if (channel_to_extract == DataSource::G1) {
        // ... (bloque de depuración sin cambios) ...
    }
    // --- END OF VISUAL DEBUG BLOCK ---

    if (crop_area.x < 0 || crop_area.y < 0 || crop_area.width <= 0 || crop_area.height <= 0 ||
        crop_area.x + crop_area.width > img_corrected.cols ||
        crop_area.y + crop_area.height > img_corrected.rows) {
        log_stream << _("Error: Invalid crop area calculated for keystone correction.") << std::endl;
        return {};
    }
    
    return img_corrected(crop_area).clone();
}

cv::Mat CreateFinalDebugImage(const cv::Mat& overlay_image, double max_pixel_value)
{
    if (overlay_image.empty()) {
        return {};
    }

    double normalization_value = max_pixel_value;
    // --- INICIO DE LA MODIFICACIÓN ---
    // If max_pixel_value is 0 (no valid patches), find the max value in the image itself for normalization.
    if (normalization_value <= 0) {
        cv::minMaxLoc(overlay_image, nullptr, &normalization_value, nullptr, nullptr);
    }

    // If there's still no valid value (e.g., black image), we cannot proceed.
    if (normalization_value <= 0) {
        return {};
    }
    // --- FIN DE LA MODIFICACIÓN ---

    // 1. Normalize using the determined value.
    cv::Mat normalized_image = overlay_image / normalization_value;

    // 2. Clamp values to the [0.0, 1.0] range.
    cv::threshold(normalized_image, normalized_image, 1.0, 1.0, cv::THRESH_TRUNC);
    cv::threshold(normalized_image, normalized_image, 0.0, 0.0, cv::THRESH_TOZERO);

    // 3. Apply standard 2.2 gamma correction for visibility.
    cv::Mat gamma_corrected_image;
    cv::pow(normalized_image, 1.0 / 2.2, gamma_corrected_image);

    return gamma_corrected_image;
}

cv::Mat DrawCornerMarkers(const cv::Mat& image, const std::vector<cv::Point2d>& corners)
{
    cv::Mat color_image;
    cv::cvtColor(image, color_image, cv::COLOR_GRAY2BGR);

    #if DYNA_RANGE_DEBUG_MODE == 1
    const cv::Scalar marker_color = cv::Scalar(
        DynaRange::Debug::CORNER_MARKER_COLOR[0],
        DynaRange::Debug::CORNER_MARKER_COLOR[1],
        DynaRange::Debug::CORNER_MARKER_COLOR[2]
    );
    #else
    const cv::Scalar marker_color = cv::Scalar(1.0, 1.0, 1.0); // White
    #endif

    for (const auto& point : corners) {
        cv::drawMarker(color_image, point, marker_color, cv::MARKER_CROSS, 40, 2);
    }
    return color_image;
}
