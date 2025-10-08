// File: src/DynaRangeGuiApp.cpp
/**
 * @file src/DynaRangeGuiApp.cpp
 * @brief Main entry point for the wxWidgets GUI version of the dynaRange application.
 */
#include "DynaRangeGuiApp.hpp"
#include "DynaRangeFrame.hpp"
#include "../core/utils/LocaleManager.hpp"
#include "../core/utils/PathManager.hpp"
#include <wx/image.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <cstdlib> // For std::getenv
#include <wx/webview.h>

// This macro creates the main() function for the GUI application
wxIMPLEMENT_APP(DynaRangeGuiApp);

bool DynaRangeGuiApp::OnInit() {
    // 1. Determine the language to use.
    int lang = wxLANGUAGE_DEFAULT;
    const char* lang_env = std::getenv("LANGUAGE");
    if (lang_env) {
        wxString lang_str(lang_env);
        const wxLanguageInfo* lang_info = wxLocale::FindLanguageInfo(lang_str);
        if (lang_info) {
            lang = lang_info->Language;
        }
    }

    // 2. Initialize the wxLocale system with the chosen language.
    m_locale.Init(lang);

    // 3. Tell wxWidgets where to find our translation files (.mo).
    PathManager path_manager(ProgramOptions{});
    wxLocale::AddCatalogLookupPathPrefix(path_manager.GetLocaleDirectory().wstring());

    // 4. Load our specific translation catalog.
    m_locale.AddCatalog("dynaRange");

    // 5. Manage the numeric locale using the dedicated manager.
    static LocaleManager locale_manager;

    // 6. Initialize image handlers and create the main window.
    wxImage::AddHandler(new wxPNGHandler());

    // Explicitly initialize the WebView backend.
    // On Windows, we prioritize the modern Edge (WebView2) backend.
#ifdef __WXMSW__
    if (wxWebView::IsBackendAvailable(wxWebViewBackendEdge)) {
        wxWebView::New(wxWebViewBackendEdge);
    } else {
        // Fallback for systems without the WebView2 runtime.
        wxLogWarning("Microsoft Edge WebView2 backend not found. Falling back to the IE backend. SVG rendering might not work correctly.");
        wxWebView::New();
    }
#else
    // For other platforms (Linux/macOS), the default is already the best available (WebKit).
    wxWebView::New();
#endif

    DynaRangeFrame* frame = new DynaRangeFrame(nullptr);
    frame->Show(true);
    return true;
}