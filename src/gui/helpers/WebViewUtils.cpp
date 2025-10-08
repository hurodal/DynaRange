// File: src/gui/helpers/WebViewUtils.cpp
/**
 * @file WebViewUtils.cpp
 * @brief Implements utility functions for wxWebView.
 */
#include "WebViewUtils.hpp"

namespace WebViewUtils {

std::string CreateHtmlForImage(const wxString& imageUrl) {
    // Use a raw string literal for clean HTML and CSS.
    std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        html, body {
            margin: 0;
            padding: 0;
            width: 100%;
            height: 100%;
            overflow: hidden;
            display: flex;
            justify-content: center;
            align-items: center;
            background-color: #ECECEC;
        }
        img {
            max-width: 100%;
            max-height: 100%;
            object-fit: contain;
        }
    </style>
</head>
<body>
    <img src=")" + std::string(imageUrl.mb_str()) + R"(" alt="Image Preview">
</body>
</html>
)";
    return html;
}

} // namespace WebViewUtils