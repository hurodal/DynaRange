// File: src/core/graphics/ImageProcessing.cpp
/**
 * @file core/graphics/ImageProcessing.cpp
 * @brief Implements high-level image processing orchestration and utilities.
 */
#include "ImageProcessing.hpp"
#include "geometry/KeystoneCorrection.hpp"
#include "../DebugConfig.hpp"
#include "../io/raw/RawFile.hpp"
#include "../utils/Formatters.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#define _(string) gettext(string)

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
    switch (channel_to_extract) {
        case DataSource::R:
            for (int r = 0; r < imgBayer.rows; ++r) { for (int c = 0; c < imgBayer.cols; ++c) { imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2); } }
            break;
        case DataSource::G1:
            for (int r = 0; r < imgBayer.rows; ++r) { for (int c = 0; c < imgBayer.cols; ++c) { imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2 + 1); } }
            break;
        case DataSource::G2:
            for (int r = 0; r < imgBayer.rows; ++r) { for (int c = 0; c < imgBayer.cols; ++c) { imgBayer.at<float>(r, c) = img_float.at<float>(r * 2 + 1, c * 2); } }
            break;
        case DataSource::B:
            for (int r = 0; r < imgBayer.rows; ++r) { for (int c = 0; c < imgBayer.cols; ++c) { imgBayer.at<float>(r, c) = img_float.at<float>(r * 2 + 1, c * 2 + 1); } }
            break;
        default:
            return {};
    }
    
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
        // 1. Save the UNCORRECTED image with the original corner selection (blue trapezoid)
        cv::Mat bayer_view;
        cv::normalize(imgBayer, bayer_view, 0.0, 1.0, cv::NORM_MINMAX);
        cv::Mat source_debug_image;
        cv::cvtColor(bayer_view, source_debug_image, cv::COLOR_GRAY2BGR);
        const auto& src_pts = chart.GetCornerPoints();
        if (src_pts.size() == 4) {
            cv::line(source_debug_image, src_pts[0], src_pts[1], cv::Scalar(1.0, 0.0, 0.0), 2); // Blue
            cv::line(source_debug_image, src_pts[1], src_pts[2], cv::Scalar(1.0, 0.0, 0.0), 2);
            cv::line(source_debug_image, src_pts[2], src_pts[3], cv::Scalar(1.0, 0.0, 0.0), 2);
            cv::line(source_debug_image, src_pts[3], src_pts[0], cv::Scalar(1.0, 0.0, 0.0), 2);
        }
        cv::Mat source_debug_8u;
        source_debug_image.convertTo(source_debug_8u, CV_8U, 255.0);
        fs::path source_path = paths.GetCsvOutputPath().parent_path() / "debug_keystone_source.png";
        if (cv::imwrite(source_path.string(), source_debug_8u)) {
            log_stream << "[DEBUG] Saved source selection trapezoid to: " << source_path.string() << std::endl;
        }

        // 2. Save the CORRECTED image with the crop rectangle (red)
        log_stream << "[DEBUG] Attempting to crop with rect: x=" << crop_area.x << ", y=" << crop_area.y << ", w=" << crop_area.width << ", h=" << crop_area.height << std::endl;
        cv::Mat corrected_view;
        cv::normalize(img_corrected, corrected_view, 0.0, 1.0, cv::NORM_MINMAX);
        cv::Mat result_debug_image;
        cv::cvtColor(corrected_view, result_debug_image, cv::COLOR_GRAY2BGR);
        cv::rectangle(result_debug_image, crop_area, cv::Scalar(0, 0, 1.0), 5); // Red
        
        cv::Mat result_debug_8u;
        result_debug_image.convertTo(result_debug_8u, CV_8U, 255.0);
        fs::path result_path = paths.GetCsvOutputPath().parent_path() / "debug_keystone_result.png";
        if (cv::imwrite(result_path.string(), result_debug_8u)) {
            log_stream << "[DEBUG] Saved corrected image with crop rectangle to: " << result_path.string() << std::endl;
        }
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
    if (overlay_image.empty() || max_pixel_value <= 0) {
        return {};
    }

    // 1. Normalize using the max value calculated *before* drawing patches.
    cv::Mat normalized_image = overlay_image / max_pixel_value;

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

std::map<DataSource, cv::Mat> PrepareAllBayerChannels(
    const RawFile& raw_file,
    double dark_value,
    double saturation_value,
    const cv::Mat& keystone_params,
    const ChartProfile& chart,
    std::ostream& log_stream)
{
    std::map<DataSource, cv::Mat> prepared_channels;
    
    // Use the active image area to be consistent with PrepareChartImage.
    cv::Mat raw_img = raw_file.GetActiveRawImage();
    
    if(raw_img.empty()){
        log_stream << _("Error: Could not get raw image for: ") << raw_file.GetFilename() << std::endl;
        return prepared_channels;
    }
    cv::Mat img_float = NormalizeRawImage(raw_img, dark_value, saturation_value);
    cv::Size bayer_size(img_float.cols / 2, img_float.rows / 2);
    
    cv::Mat r_bayer(bayer_size, CV_32FC1);
    cv::Mat g1_bayer(bayer_size, CV_32FC1);
    cv::Mat g2_bayer(bayer_size, CV_32FC1);
    cv::Mat b_bayer(bayer_size, CV_32FC1);
    
    for (int r = 0; r < bayer_size.height; ++r) {
        for (int c = 0; c < bayer_size.width; ++c) {
            r_bayer.at<float>(r, c)  = img_float.at<float>(r * 2, c * 2);
            g1_bayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2 + 1);
            g2_bayer.at<float>(r, c) = img_float.at<float>(r * 2 + 1, c * 2);
            b_bayer.at<float>(r, c)  = img_float.at<float>(r * 2 + 1, c * 2 + 1);
        }
    }
    
    std::map<DataSource, cv::Mat> bayer_channels = {
        {DataSource::R, r_bayer}, {DataSource::G1, g1_bayer},
        {DataSource::G2, g2_bayer}, {DataSource::B, b_bayer}
    };

    const auto& dst_pts = chart.GetDestinationPoints();
    double xtl = dst_pts[0].x;
    double ytl = dst_pts[0].y;
    double xbr = dst_pts[2].x;
    double ybr = dst_pts[2].y;
    cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
    
    for (auto const& [channel, bayer_mat] : bayer_channels) {
        cv::Mat img_corrected = DynaRange::Graphics::Geometry::UndoKeystone(bayer_mat, keystone_params);
        if (crop_area.x < 0 || crop_area.y < 0 || crop_area.width <= 0 || crop_area.height <= 0 ||
            crop_area.x + crop_area.width > img_corrected.cols ||
            crop_area.y + crop_area.height > img_corrected.rows) {
            log_stream << _("Error: Invalid crop area for channel ") << Formatters::DataSourceToString(channel) << std::endl;
            prepared_channels[channel] = cv::Mat();
        } else {
            prepared_channels[channel] = img_corrected(crop_area).clone();
        }
    }

    return prepared_channels;
}