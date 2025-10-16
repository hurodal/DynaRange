// File: src/core/engine/processing/CornerDetectionHandler.cpp
/**
 * @file CornerDetectionHandler.cpp
 * @brief Implements the automatic chart corner detection logic.
 */
#include "CornerDetectionHandler.hpp"
#include "../../graphics/ImageProcessing.hpp"
#include "../../graphics/detection/ChartCornerDetector.hpp"
#include "../../io/OutputWriter.hpp"
#include "../../DebugConfig.hpp"
#include "../Constants.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>

#define _(string) gettext(string)

namespace fs = std::filesystem;

namespace DynaRange::Engine::Processing {

std::optional<std::vector<cv::Point2d>> AttemptAutomaticCornerDetection(
    const RawFile& source_raw_file,
    const std::vector<double>& chart_coords,
    double dark_value,
    double saturation_value,
    const PathManager& paths,
    std::ostream& log_stream)
{
    if (chart_coords.empty() && source_raw_file.IsLoaded()) {
        log_stream << _("Manual coordinates not provided, attempting automatic corner detection...") << std::endl;
        
        // Use the active image area to exclude masked pixels.
        cv::Mat raw_img = source_raw_file.GetActiveRawImage();
        
        cv::Mat img_float = NormalizeRawImage(raw_img, dark_value, saturation_value);

        int bayer_rows = img_float.rows / 2;
        int bayer_cols = img_float.cols / 2;

        cv::Mat g1_bayer = cv::Mat::zeros(bayer_rows, bayer_cols, CV_32FC1);
        for (int r = 0; r < bayer_rows; ++r) {
            for (int c = 0; c < bayer_cols; ++c) {
                g1_bayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2 + 1);
            }
        }

        cv::threshold(g1_bayer, g1_bayer, 0.0, 0.0, cv::THRESH_TOZERO);
        std::optional<std::vector<cv::Point2d>> detected_corners_opt = DynaRange::Graphics::Detection::DetectChartCorners(g1_bayer, log_stream);
        
        #if DYNA_RANGE_DEBUG_MODE == 1
        if (DynaRange::Debug::ENABLE_CORNER_DETECTION_DEBUG && detected_corners_opt.has_value()) {
            log_stream << "  - [DEBUG] Saving corner detection visual confirmation to 'debug_corners_detected.png'..." << std::endl;
            cv::Mat viewable_image;
            cv::normalize(g1_bayer, viewable_image, 0.0, 1.0, cv::NORM_MINMAX);
            cv::Mat image_with_markers = DrawCornerMarkers(viewable_image, *detected_corners_opt);
            cv::Mat final_debug_image;
            cv::pow(image_with_markers, 1.0 / 2.2, final_debug_image);
            fs::path debug_path = paths.GetCsvOutputPath().parent_path() / "debug_corners_detected.png";
            OutputWriter::WriteDebugImage(final_debug_image, debug_path, log_stream);
        }
        #endif

        if (detected_corners_opt.has_value()) {
            std::vector<cv::Point2f> corners_float;
            for (const auto& pt : *detected_corners_opt) {
                corners_float.push_back(cv::Point2f(static_cast<float>(pt.x), static_cast<float>(pt.y)));
            }
            double total_image_area = static_cast<double>(g1_bayer.cols * g1_bayer.rows);
            double detected_chart_area = cv::contourArea(corners_float);
            double area_percentage = (detected_chart_area / total_image_area);
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
    return std::nullopt;
}

} // namespace DynaRange::Engine::Processing
