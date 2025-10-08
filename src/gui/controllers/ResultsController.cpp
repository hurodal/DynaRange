// File: gui/ResultsController.cpp
/**
 * @file gui/ResultsController.cpp
 * @brief Implements the ResultsController class.
 */
#include "ResultsController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../helpers/ResultsGridManager.hpp"
#include "../helpers/WebViewUtils.hpp"
#include "../utils/PathManager.hpp"
#include <wx/webview.h>
#include <wx/log.h>
#include <filesystem>
#include <string>
#include <librsvg/rsvg.h>

namespace fs = std::filesystem;

// Anonymous namespace for internal helper functions.
namespace {

constexpr const char* ONLINE_URL = "https://www.overfitting.net/2025/07/rango-dinamico-de-un-sensor-de-imagen.html";

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
    if (path.empty()) {
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        return;
    }

    fs::path imagePath(path);
    if (!fs::exists(imagePath)) {
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph (Image not found): ") + wxString(imagePath.filename().string()));
        LoadDefaultContent();
        return;
    }

    m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph: ") + wxString(imagePath.filename().string()));

    // Differentiate between SVG and PNG
    if (imagePath.extension() == ".svg") {
        // --- SVG Rendering Path ---
        m_frame->m_resultsWebView->Hide(); // Hide the webview
        m_frame->m_svgCanvasPanel->Show(); // Show our SVG canvas

        // Free previous SVG handle if it exists
        if (m_frame->m_rsvgHandle) {
            g_object_unref(m_frame->m_rsvgHandle);
            m_frame->m_rsvgHandle = nullptr;
        }

        // Load the new SVG file
        GError* error = nullptr;
        m_frame->m_rsvgHandle = rsvg_handle_new_from_file(path.c_str(), &error);
        if (error) {
            wxLogError("Failed to load SVG file: %s", error->message);
            g_error_free(error);
            return;
        }
        
        // Trigger a repaint of the panel
        m_frame->m_svgCanvasPanel->Refresh();
        m_frame->m_svgCanvasPanel->Update();

    } else { // PNG or other formats
        // --- PNG Rendering Path (fallback to WebView) ---
        m_frame->m_svgCanvasPanel->Hide(); // Hide our SVG canvas
        m_frame->m_resultsWebView->Show(); // Show the webview

        wxString image_file_url = wxString("file://") + wxString(fs::absolute(imagePath).string());
        std::string html = WebViewUtils::CreateHtmlForImage(image_file_url);
        m_frame->m_resultsWebView->SetPage(html, "");
    }
    m_frame->m_webViewPlaceholderPanel->Layout();
}


void ResultsController::LoadDefaultContent() {
    if (!m_webView) return;
    // The only responsibility of this function is to load the URL.
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
        m_frame->m_generateGraphStaticText->SetLabel(_("Starting analysis... Please wait."));
        
        // Ensure webview is visible for default content, hide SVG canvas
        m_frame->m_svgCanvasPanel->Hide();
        m_frame->m_resultsWebView->Show();
        m_frame->m_webViewPlaceholderPanel->Layout();

        LoadDefaultContent();
        m_frame->m_processingGauge->Show();
    } else {
        m_frame->m_processingGauge->Hide();
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        m_frame->m_csvOutputStaticText->Show();
        m_frame->m_cvsGrid->Show();
    }
    m_frame->m_rightPanel->Layout();
    m_frame->m_rightPanel->Refresh();
}

void ResultsController::OnSplitterSashDClick(wxSplitterEvent& event) {
    if (m_frame->m_splitterResults->IsSplit()) {
        m_lastSashPosition = event.GetSashPosition();
        m_frame->m_splitterResults->Unsplit(m_frame->m_leftPanel);
    } else {
        m_frame->m_splitterResults->SplitVertically(m_frame->m_leftPanel, m_frame->m_rightPanel, m_lastSashPosition);
    }
}
