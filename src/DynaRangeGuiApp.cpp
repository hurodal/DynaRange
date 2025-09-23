// File: src/DynaRangeGuiApp.cpp
/**
 * @file src/DynaRangeGuiApp.cpp
 * @brief Main entry point for the wxWidgets GUI version of the dynaRange application.
 */
#include "DynaRangeGuiApp.hpp"
#include "gui/DynaRangeFrame.hpp"
#include <wx/image.h>
#include <clocale>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <cstdlib> // For std::getenv

// This macro creates the main() function for the GUI application
wxIMPLEMENT_APP(DynaRangeGuiApp);

bool DynaRangeGuiApp::OnInit() {
    // 1. Determine the language to use.
    // By default, try to use the system's language.
    int lang = wxLANGUAGE_DEFAULT;

    // Check if the LANGUAGE environment variable is set to override the default.
    // This allows testing different languages from the terminal (e.g., LANGUAGE=es).
    const char* lang_env = std::getenv("LANGUAGE");
    if (lang_env) {
        wxString lang_str(lang_env);
        const wxLanguageInfo* lang_info = wxLocale::FindLanguageInfo(lang_str);
        if (lang_info) {
            lang = lang_info->Language; // If found, use the specified language.
        }
    }

    // 2. Initialize the wxLocale system with the chosen language.
    m_locale.Init(lang);

    // 3. Tell wxWidgets where to find our translation files (.mo).
    // It will look in a 'locale' directory relative to the executable.
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fn(exePath);
    wxString localeDir = fn.GetPath() + wxFILE_SEP_PATH + "locale";
    wxLocale::AddCatalogLookupPathPrefix(localeDir);

    // 4. Load our specific translation catalog.
    m_locale.AddCatalog("dynaRange");
    
    // 5. Set the numeric locale to "C" for consistent number parsing (e.g., "123.45").
    std::setlocale(LC_NUMERIC, "C");

    // 6. Initialize image handlers and create the main window.
    wxImage::AddHandler(new wxPNGHandler);
    DynaRangeFrame* frame = new DynaRangeFrame(nullptr);
    frame->Show(true);
    
    return true;
}