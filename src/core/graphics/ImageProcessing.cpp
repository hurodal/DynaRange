// File: src/core/graphics/ImageProcessing.cpp
/**
 * @file core/ImageProcessing.cpp
 * @brief Implements geometric image processing functions.
 */
#include "ImageProcessing.hpp"
#include "../io/RawFile.hpp"
#include "../math/Math.hpp"
#include "../DebugConfig.hpp"
#include "../Constants.hpp"
#include "../utils/Formatters.hpp"
#include <libintl.h>
#include <opencv2/imgproc.hpp>
#include <optional>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

#define _(string) gettext(string)

Eigen::VectorXd CalculateKeystoneParams(const std::vector<cv::Point2d>& src_points, const std::vector<cv::Point2d>& dst_points) {
    Eigen::Matrix<double, 8, 8> A;
    Eigen::Vector<double, 8> b;
    for (int i = 0; i < 4; ++i) {
        const auto& xu = src_points[i].x;
        const auto& yu = src_points[i].y;
        const auto& xd = dst_points[i].x; const auto& yd = dst_points[i].y;
        A.row(2 * i)     << xd, yd, 1, 0,  0,  0, -xd * xu, -yd * xu;
        A.row(2 * i + 1) << 0,  0,  0, xd, yd, 1, -xd * yu, -yd * yu;
        b(2 * i) = xu; b(2 * i + 1) = yu;
    }
    return A.colPivHouseholderQr().solve(b);
}

/**
 * @brief Applies an inverse keystone correction to an image using Nearest Neighbor interpolation.
 * @param imgSrc The source image to be corrected.
 * @param k An Eigen::VectorXd containing the 8 transformation parameters.
 * @return A new cv::Mat containing the rectified image.
 */
cv::Mat UndoKeystone(const cv::Mat& imgSrc, const Eigen::VectorXd& k) {
    int DIMX = imgSrc.cols;
    int DIMY = imgSrc.rows;
    cv::Mat imgCorrected = cv::Mat::zeros(DIMY, DIMX, CV_32FC1);
    for (int y = 0; y < DIMY; ++y) {
        for (int x = 0; x < DIMX; ++x) {
            // RESTORED LOGIC: Use 1-based coordinates for the calculation.
            double xd = x + 1.0, yd = y + 1.0;
            double denom = k(6) * xd + k(7) * yd + 1;
            if (std::abs(denom) < 1e-9) continue;

            // RESTORED LOGIC: Subtract 1.0 from the coordinate before rounding.
            double xu = (k(0) * xd + k(1) * yd + k(2)) / denom - 1.0;
            double yu = (k(3) * xd + k(4) * yd + k(5)) / denom - 1.0;

            // RESTORED LOGIC: Round the final coordinate to get the source pixel.
            int x_src = static_cast<int>(round(xu));
            int y_src = static_cast<int>(round(yu));

            if (x_src >= 0 && x_src < DIMX && y_src >= 0 && y_src < DIMY) {
                imgCorrected.at<float>(y, x) = imgSrc.at<float>(y_src, x_src);
            }
        }
    }
    return imgCorrected;
}

cv::Mat PrepareChartImage(
    const RawFile& raw_file, 
    const ProgramOptions& opts, 
    const Eigen::VectorXd& keystone_params,
    const ChartProfile& chart, 
    std::ostream& log_stream,
    DataSource channel_to_extract) 
{
    cv::Mat raw_img = raw_file.GetRawImage();
    if(raw_img.empty()){
        log_stream << _("Error: Could not get raw image for: ") << raw_file.GetFilename() << std::endl;
        return {};
    }
    cv::Mat img_float = NormalizeRawImage(raw_img, opts.dark_value, opts.saturation_value);

    cv::Mat imgBayer(img_float.rows / 2, img_float.cols / 2, CV_32FC1);
    // The switch now uses the function parameter instead of a global constant.
    switch (channel_to_extract) {
        case DataSource::R:
            for (int r = 0; r < imgBayer.rows; ++r) {
                for (int c = 0; c < imgBayer.cols; ++c) {
                    imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2);
                }
            }
            break;
        case DataSource::G1:
            for (int r = 0; r < imgBayer.rows; ++r) {
                for (int c = 0; c < imgBayer.cols; ++c) {
                    imgBayer.at<float>(r, c) = img_float.at<float>(r * 2, c * 2 + 1);
                }
            }
            break;
        case DataSource::G2:
            for (int r = 0; r < imgBayer.rows; ++r) {
                for (int c = 0; c < imgBayer.cols; ++c) {
                    imgBayer.at<float>(r, c) = img_float.at<float>(r * 2 + 1, c * 2);
                }
            }
            break;
        case DataSource::B:
            for (int r = 0; r < imgBayer.rows; ++r) {
                for (int c = 0; c < imgBayer.cols; ++c) {
                    imgBayer.at<float>(r, c) = img_float.at<float>(r * 2 + 1, c * 2 + 1);
                }
            }
            break;
        case DataSource::AVG: {
            cv::Mat r_channel(imgBayer.size(), CV_32FC1);
            cv::Mat g1_channel(imgBayer.size(), CV_32FC1);
            cv::Mat g2_channel(imgBayer.size(), CV_32FC1);
            cv::Mat b_channel(imgBayer.size(), CV_32FC1);

            for (int r = 0; r < imgBayer.rows; ++r) {
                for (int c = 0; c < imgBayer.cols; ++c) {
                    r_channel.at<float>(r, c)  = img_float.at<float>(r * 2, c * 2);
                    g1_channel.at<float>(r, c) = img_float.at<float>(r * 2, c * 2 + 1);
                    g2_channel.at<float>(r, c) = img_float.at<float>(r * 2 + 1, c * 2);
                    b_channel.at<float>(r, c)  = img_float.at<float>(r * 2 + 1, c * 2 + 1);
                }
            }
            imgBayer = (r_channel + g1_channel + g2_channel + b_channel) / 4.0;
            break;
        }
    }
    
    cv::Mat img_corrected = UndoKeystone(imgBayer, keystone_params);
    const auto& dst_pts = chart.GetDestinationPoints();
    
    // TEMPORARY REVERSION: Use the old, simple bounding box crop logic to match the old version.
    double xtl = dst_pts[0].x;
    double ytl = dst_pts[0].y;
    double xbr = dst_pts[2].x;
    double ybr = dst_pts[2].y;
    cv::Rect crop_area(round(xtl), round(ytl), round(xbr - xtl), round(ybr - ytl));
    
    if (crop_area.x < 0 || crop_area.y < 0 || crop_area.width <= 0 || crop_area.height <= 0 ||
        crop_area.x + crop_area.width > img_corrected.cols ||
        crop_area.y + crop_area.height > img_corrected.rows) {
        log_stream << _("Error: Invalid crop area calculated for keystone correction.") << std::endl;
        return {};
    }
    
    // The clone() is critical to return a valid copy, not a pointer to a temporary image.
    cv::Mat final_image = img_corrected(crop_area).clone();

    //// --- PUNTO DE CONTROL Nº2 ---
    //#if DEBUG_IA_ON == 1
    //    cv::Scalar mean_val, stddev_val;
    //    cv::meanStdDev(final_image, mean_val, stddev_val);
    //    log_stream << "\n--- DEBUG IA: Punto de Control 2 (Imagen Preparada Final) ---\n";
    //    log_stream << "Fichero: " << fs::path(raw_file.GetFilename()).filename().string() << "\n";
    //    log_stream << "Canal: " << Formatters::DataSourceToString(channel_to_extract) << "\n";
    //    log_stream << "Media de la imagen final:   " << std::fixed << std::setprecision(8) << mean_val[0] << "\n";
    //    log_stream << "StdDev de la imagen final: " << std::fixed << std::setprecision(8) << stddev_val[0] << "\n";
    //    log_stream << "--------------------------------------------------------\n";
    //    log_stream << "--- DEBUG IA: Finalizando el programa en el punto de control. ---\n" << std::endl;
    //    //exit(0);
    //#endif
    //// --- FIN PUNTO DE CONTROL Nº2 ---

    return final_image;
}

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

std::optional<std::vector<cv::Point2d>> DetectChartCorners(const cv::Mat& bayer_image, std::ostream& log_stream)
{
    if (bayer_image.empty()) return std::nullopt;
    const int DIMX = bayer_image.cols;
    const int DIMY = bayer_image.rows;
    std::vector<cv::Point2d> detected_points;

    std::vector<cv::Rect> sectors = {
        {0, 0, DIMX / 2, DIMY / 2},
        {0, DIMY / 2, DIMX / 2, DIMY / 2},
        {DIMX / 2, DIMY / 2, DIMX / 2, DIMY / 2},
        {DIMX / 2, 0, DIMX / 2, DIMY / 2}
    };
    const double diag = std::sqrt(static_cast<double>(DIMX * DIMX + DIMY * DIMY));
    const double radius = diag * 0.01;
    const double circle_area = M_PI * radius * radius;
    const double quadrant_area = (DIMX / 2.0) * (DIMY / 2.0);
    const double quantile_fraction = circle_area / quadrant_area;
    const double quantile_threshold = 1.0 - (quantile_fraction / 4.0);

    for (const auto& sector_rect : sectors) {
        cv::Mat quadrant = bayer_image(sector_rect).clone();
        if (quadrant.empty() || quadrant.total() == 0) continue;

        std::vector<double> pixels;
        quadrant.reshape(1, 1).convertTo(pixels, CV_64F);
        double brightness_q_threshold = CalculateQuantile(pixels, quantile_threshold);

        cv::Mat mask;
        cv::threshold(quadrant, mask, brightness_q_threshold, 1.0, cv::THRESH_BINARY);
        mask.convertTo(mask, CV_8U);
        
        std::vector<cv::Point> bright_pixels;
        cv::findNonZero(mask, bright_pixels);
        if (bright_pixels.empty()) {
            log_stream << _("Warning: No corner circle found in one of the quadrants.") << std::endl;
            return std::nullopt;
        }

        std::vector<int> x_coords, y_coords;
        x_coords.reserve(bright_pixels.size());
        y_coords.reserve(bright_pixels.size());
        for(const auto& p : bright_pixels) {
            x_coords.push_back(p.x);
            y_coords.push_back(p.y);
        }
        std::sort(x_coords.begin(), x_coords.end());
        std::sort(y_coords.begin(), y_coords.end());
        
        double median_x_local = static_cast<double>(x_coords[x_coords.size() / 2]);
        double median_y_local = static_cast<double>(y_coords[y_coords.size() / 2]);
        
        double median_x = median_x_local + sector_rect.x;
        double median_y = median_y_local + sector_rect.y;
        detected_points.emplace_back(median_x, median_y);
    }
    
    if (detected_points.size() == 4) {
        cv::Point2d tl = detected_points[0];
        cv::Point2d bl = detected_points[1];
        cv::Point2d br = detected_points[2];
        cv::Point2d tr = detected_points[3];
        return std::vector<cv::Point2d>{tl, bl, br, tr};
    }

    return std::nullopt;
}

cv::Mat UndoKeystoneColor(const cv::Mat& imgSrc, const Eigen::VectorXd& k) {
    int DIMX = imgSrc.cols;
    int DIMY = imgSrc.rows;
    // Create a destination image with 3 channels for color (CV_8UC3)
    cv::Mat imgCorrected = cv::Mat::zeros(DIMY, DIMX, CV_8UC3);

    for (int y = 0; y < DIMY; ++y) {
        for (int x = 0; x < DIMX; ++x) {
            double xd = x + 1.0, yd = y + 1.0;
            double denom = k(6) * xd + k(7) * yd + 1;
            if (std::abs(denom) < 1e-9) continue;

            double xu = (k(0) * xd + k(1) * yd + k(2)) / denom - 1;
            double yu = (k(3) * xd + k(4) * yd + k(5)) / denom - 1;
            
            int x_src = static_cast<int>(round(xu));
            int y_src = static_cast<int>(round(yu));

            if (x_src >= 0 && x_src < DIMX && y_src >= 0 && y_src < DIMY) {
                // Access pixels using cv::Vec3b for 3-channel 8-bit images
                imgCorrected.at<cv::Vec3b>(y, x) = imgSrc.at<cv::Vec3b>(y_src, x_src);
            }
        }
    }
    return imgCorrected;
}

cv::Mat DrawCornerMarkers(const cv::Mat& image, const std::vector<cv::Point2d>& corners)
{
    // Convierte la imagen de un canal a una de 3 canales para poder dibujar en color.
    cv::Mat color_image;
    cv::cvtColor(image, color_image, cv::COLOR_GRAY2BGR);

    #if DYNA_RANGE_DEBUG_MODE == 1
    // El color se define ahora en DebugConfig.hpp
    const cv::Scalar marker_color = cv::Scalar(
        DynaRange::Debug::CORNER_MARKER_COLOR[0],
        DynaRange::Debug::CORNER_MARKER_COLOR[1],
        DynaRange::Debug::CORNER_MARKER_COLOR[2]
    );
    #else
    // Color por defecto si la depuración está desactivada (aunque este código no se compilará)
    const cv::Scalar marker_color = cv::Scalar(1.0, 1.0, 1.0); // Blanco
    #endif

    for (const auto& point : corners) {
        // Dibuja una cruz (+) del color especificado en cada coordenada.
        cv::drawMarker(color_image, point, marker_color, cv::MARKER_CROSS, 40, 2);
    }
    return color_image;
}