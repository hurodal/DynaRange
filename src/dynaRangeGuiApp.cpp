#include <wx/wx.h>
#include "gui/DynaRangeFrame.hpp"
#include <clocale>

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

    // Crea y muestra la ventana principal
    DynaRangeFrame *frame = new DynaRangeFrame(NULL);
    frame->Show(true);
    return true;
}