#include <wx/wx.h>
#include <wx/image.h>
#include <clocale>
#include "gui/DynaRangeFrame.hpp"


// Main application class
class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

wxIMPLEMENT_APP(MyApp);

// This function is executed when the program starts
bool MyApp::OnInit() {
    // It is important to set a standard locale for number conversion
    std::setlocale(LC_ALL, "C");

    // Initialize all image format handlers (PNG, JPG, etc.)
    wxInitAllImageHandlers();
    
    // Create and show the main window
    DynaRangeFrame *frame = new DynaRangeFrame(NULL);
    frame->Show(true);
    return true;
}