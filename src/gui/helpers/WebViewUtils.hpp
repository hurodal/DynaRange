// File: src/gui/helpers/WebViewUtils.hpp
/**
 * @file WebViewUtils.hpp
 * @brief Declares utility functions for wxWebView.
 */
#pragma once
#include <wx/string.h>
#include <string>

namespace WebViewUtils {
    /**
     * @brief Creates a self-contained HTML page to display an image responsively.
     * @param imageUrl The full URL (e.g., "file:///...") of the image to display.
     * @return A std::string containing the full HTML content.
     */
    std::string CreateHtmlForImage(const wxString& imageUrl);
}