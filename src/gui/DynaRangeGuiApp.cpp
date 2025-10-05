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
    
    // 3. Tell wxWidgets where to find our translation files (.mo)
    // using the centralized PathManager.
    PathManager path_manager(ProgramOptions{});
    wxLocale::AddCatalogLookupPathPrefix(path_manager.GetLocaleDirectory().wstring());

    // 4. Load our specific translation catalog.
    m_locale.AddCatalog("dynaRange");

    // 5. Manage the numeric locale using the dedicated manager.
    static LocaleManager locale_manager;

    // 6. Initialize image handlers and create the main window.
    wxImage::AddHandler(new wxPNGHandler());
    wxWebView::New(); // Add this line to initialize the webview backend

    DynaRangeFrame* frame = new DynaRangeFrame(nullptr);
    frame->Show(true);
    return true;
}