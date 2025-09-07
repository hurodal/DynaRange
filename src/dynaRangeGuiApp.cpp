#include <wx/wx.h>
#include <wx/image.h>
#include <clocale>
#include "gui/DynaRangeFrame.hpp"


// Clase principal de la aplicación
class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

wxIMPLEMENT_APP(MyApp);

// Esta función se ejecuta al iniciar el programa
bool MyApp::OnInit() {
    // Es importante establecer un locale estándar para la conversión de números
    std::setlocale(LC_ALL, "C");

    // Inicializa todos los manejadores de formatos de imagen (PNG, JPG, etc.)
    wxInitAllImageHandlers();
    
    // Crea y muestra la ventana principal
    DynaRangeFrame *frame = new DynaRangeFrame(NULL);
    frame->Show(true);
    return true;
}