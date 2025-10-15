// File: src/gui/helpers/CvWxImageConverter.cpp
/**
 * @file CvWxImageConverter.cpp
 * @brief Implements the OpenCV to wxWidgets image conversion utility.
 */
#include "CvWxImageConverter.hpp"
#include <opencv2/imgproc.hpp>

namespace GuiHelpers {

wxImage CvMatToWxImage(const cv::Mat& mat)
{
    if (mat.empty() || mat.type() != CV_8UC3) {
        return wxImage(); // Return an invalid image
    }

    // Create a wxImage with the same dimensions.
    // The 'static_data' argument is false, so wxImage will allocate its own buffer.
    wxImage image(mat.cols, mat.rows);

    // Create a temporary cv::Mat that points to the wxImage's data buffer.
    // This allows us to use OpenCV's highly optimized cvtColor function directly.
    cv::Mat mat_rgb(mat.rows, mat.cols, CV_8UC3, image.GetData());

    // Convert the source BGR image to the destination RGB format.
    cv::cvtColor(mat, mat_rgb, cv::COLOR_BGR2RGB);

    return image;
}

} // namespace GuiHelpers