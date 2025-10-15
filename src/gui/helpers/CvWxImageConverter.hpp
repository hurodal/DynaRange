// File: src/gui/helpers/CvWxImageConverter.hpp
/**
 * @file CvWxImageConverter.hpp
 * @brief Declares a helper utility to convert between OpenCV and wxWidgets image formats.
 */
#pragma once

#include <opencv2/core/mat.hpp>
#include <wx/image.h>

namespace GuiHelpers {

/**
 * @brief Converts an OpenCV cv::Mat (BGR format) to a wxImage (RGB format).
 * @param mat The source OpenCV matrix, expected to be in CV_8UC3 format (BGR).
 * @return A wxImage containing the converted image data. Returns an invalid
 * image if the source matrix is empty or has a wrong format.
 */
wxImage CvMatToWxImage(const cv::Mat& mat);

} // namespace GuiHelpers