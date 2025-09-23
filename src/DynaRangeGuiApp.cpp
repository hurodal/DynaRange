// File: src/DynaRangeGuiApp.cpp
/**
 * @file src/DynaRangeGuiApp.cpp
 * @brief Main entry point for the wxWidgets GUI version of the dynaRange application.
 */
#include "gui/DynaRangeFrame.hpp"
#include <wx/wx.h>
#include <wx/image.h>
#include <clocale>
#include <libintl.h> 

/**
 * @class DynaRangeGuiApp
 * @brief The main application class for the GUI. Inherits from wxApp.
 */
class DynaRangeGuiApp : public wxApp {
public:
    /**
     * @brief This is the main entry point of the GUI application.
     * It is called on startup.
     * @return True to continue running the application, false to exit.
     */
    virtual bool OnInit() override;
};

// This macro creates the main() function for the GUI application
wxIMPLEMENT_APP(DynaRangeGuiApp);

bool DynaRangeGuiApp::OnInit() {
    // 1. Initialize the localization system (gettext).
    setlocale(LC_ALL, "");
    bindtextdomain("dynaRange", "locale");
    textdomain("dynaRange");

    // 2. Set the numeric locale to "C" for consistent number parsing.
    std::setlocale(LC_NUMERIC, "C");

    // Allows loading PNG images for the logo, etc.
    wxImage::AddHandler(new wxPNGHandler);

    // Create the main application window (frame)
    DynaRangeFrame* frame = new DynaRangeFrame(nullptr);
    frame->Show(true);
    
    return true;
}