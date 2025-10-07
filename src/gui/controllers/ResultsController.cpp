// File: gui/ResultsController.cpp
/**
 * @file gui/ResultsController.cpp
 * @brief Implements the ResultsController class.
 */
#include "ResultsController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../helpers/ResultsGridManager.hpp"
#include "../utils/PathManager.hpp"
#include <wx/webview.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

namespace fs = std::filesystem;

// Anonymous namespace for internal helper functions.
namespace {

constexpr const char* ONLINE_URL = "https://www.overfitting.net/2025/07/rango-dinamico-de-un-sensor-de-imagen.html";

/**
 * @brief Encodes a block of binary data into a Base64 string.
 * @param data Pointer to the input data.
 * @param len The length of the input data in bytes.
 * @return The Base64 encoded string.
 */
std::string base64_encode(const unsigned char* data, size_t len) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string ret;
    ret.reserve((len + 2) / 3 * 4);
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (len--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while ((i++ < 3))
            ret += '=';
    }

    return ret;
}

/**
 * @brief Gets the MIME type for a given file extension.
 * @param extension The file extension (e.g., ".png").
 * @return A string representing the MIME type (e.g., "image/png").
 */
std::string get_mime_type(const std::string& extension) {
    if (extension == ".png") return "image/png";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".pdf") return "application/pdf";
    return "application/octet-stream"; // Fallback generic type
}

} // end anonymous namespace

ResultsController::ResultsController(DynaRangeFrame* frame)
    : m_frame(frame), m_lastSashPosition(350)
{
    // This now points to the manually created wxWebView in the frame.
    m_webView = m_frame->m_resultsWebView;
    m_gridManager = std::make_unique<ResultsGridManager>(m_frame->m_cvsGrid);
    m_frame->m_splitterResults->SetSashPosition(m_lastSashPosition);
}

ResultsController::~ResultsController() = default;

void ResultsController::LoadGraphImage(const std::string& path) {
    if (path.empty() || !m_webView) {
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        return;
    }

    fs::path imagePath(path);
    if (!fs::exists(imagePath)) {
        m_webView->LoadURL("about:blank");
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph (Image not found): ") + wxString(imagePath.filename().string()));
        return;
    }

    // --- NEW LOGIC: IN-MEMORY HTML WRAPPER ---
    std::string ext = imagePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    std::string mime_type = get_mime_type(ext);

    // 1. Read the entire file into a memory buffer.
    std::ifstream file(imagePath, std::ios::binary);
    if (!file) {
        m_webView->LoadURL("about:blank");
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph (Could not read file): ") + wxString(imagePath.filename().string()));
        return;
    }
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

    // 2. Base64 encode the buffer.
    std::string encoded_data = base64_encode(buffer.data(), buffer.size());

    // 3. Create the Data URI.
    std::string data_uri = "data:" + mime_type + ";base64," + encoded_data;

    // 4. Construct the HTML content as a string.
    std::stringstream html_ss;
    html_ss << "<!DOCTYPE html>\n"
            << "<html lang=\"en\">\n<head>\n"
            << "  <meta charset=\"UTF-8\">\n"
            << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
            << "  <title>Image Viewer</title>\n"
            << "  <style>\n"
            << "    body, html { margin: 0; padding: 0; width: 100%; height: 100%; overflow: hidden; display: flex; justify-content: center; align-items: center; background-color: #ECECEC; }\n";

    if (ext == ".pdf") {
        html_ss << "    embed { width: 100%; height: 100%; }\n";
    } else {
        html_ss << "    img { max-width: 100%; max-height: 100%; object-fit: contain; }\n";
    }

    html_ss << "  </style>\n"
            << "</head>\n<body>\n";

    if (ext == ".pdf") {
        html_ss << "  <embed src=\"" << data_uri << "\" type=\"" << mime_type << "\">\n";
    } else {
        html_ss << "  <img src=\"" << data_uri << "\" alt=\"Generated Plot\">\n";
    }

    html_ss << "</body>\n</html>";

    // 5. Load the HTML string directly into the web view.
    m_webView->SetPage(html_ss.str(), "");
    m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph: ") + wxString(imagePath.filename().string()));
}

void ResultsController::LoadDefaultContent() {
    if (!m_webView) return;
    // La única responsabilidad de esta función es cargar la URL.
    m_webView->LoadURL(ONLINE_URL);
}

void ResultsController::LoadLogoImage() {
    if (!m_webView) return;

    // Use PathManager to locate the asset directory correctly.
    ProgramOptions opts; // An empty opts is sufficient for PathManager
    PathManager path_manager(opts);
    fs::path logo_path = path_manager.GetAssetPath("assets/images/logo.png");

    if (fs::exists(logo_path)) {
        wxString url = "file://" + wxString(fs::absolute(logo_path).string());
        m_webView->LoadURL(url);
        m_frame->m_generateGraphStaticText->SetLabel(_("Welcome to Dynamic Range Calculator"));
    } else {
        m_webView->LoadURL("about:blank");
        m_frame->m_generateGraphStaticText->SetLabel(_("Welcome (logo.png not found)"));
    }
}

bool ResultsController::DisplayResults(const std::string& csv_path) {
    return m_gridManager->LoadFromCsv(csv_path);
}


void ResultsController::SetUiState(bool is_processing) {
    if (is_processing) {
        m_frame->m_csvOutputStaticText->Hide();
        m_frame->m_cvsGrid->Hide();
        // 1. Ponemos el mensaje de estado inicial correcto.
        m_frame->m_generateGraphStaticText->SetLabel(_("Starting analysis... Please wait."));

        // 2. Llamamos a la función que carga la página web.
        //    Como ya no modifica la etiqueta, el mensaje anterior se mantiene
        //    y será actualizado después por OnWorkerUpdate.
        LoadDefaultContent();

        m_frame->m_processingGauge->Show();

        m_frame->m_rightPanel->Layout();
        m_frame->m_rightPanel->Refresh();
    } else {
        m_frame->m_processingGauge->Hide();
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        m_frame->m_csvOutputStaticText->Show();
        m_frame->m_cvsGrid->Show();
        m_frame->m_rightPanel->Layout();
        m_frame->m_rightPanel->Refresh();
    }
}

void ResultsController::OnSplitterSashDClick(wxSplitterEvent& event) {
    if (m_frame->m_splitterResults->IsSplit()) {
        m_lastSashPosition = event.GetSashPosition();
        m_frame->m_splitterResults->Unsplit(m_frame->m_leftPanel);
    } else {
        m_frame->m_splitterResults->SplitVertically(m_frame->m_leftPanel, m_frame->m_rightPanel, m_lastSashPosition);
    }
}

