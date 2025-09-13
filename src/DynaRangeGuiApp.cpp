// File: src/DynaRangeGuiApp.cpp
/**
 * @file DynaRangeGuiApp.cpp
 * @brief Main entry point for the wxWidgets GUI version of the dynaRange application.
 */
#include "gui/DynaRangeFrame.hpp"
#include <wx/wx.h>
#include <wx/image.h>
#include <clocale>

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
    // It is important to set a standard locale for number conversion consistency.
    std::setlocale(LC_ALL, "C");

    // Allows loading PNG images for the logo, etc.
    wxImage::AddHandler(new wxPNGHandler);

    // Create the main application window (frame)
    DynaRangeFrame* frame = new DynaRangeFrame(nullptr);
    frame->Show(true);
    
    return true;
}
