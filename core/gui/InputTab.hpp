// core/gui/InputTab.hpp
#pragma once
#include <wx/wx.h>
#include <wx/filepicker.h>
#include <wx/listbox.h>
#include "../../core/arguments.hpp"

// Clase que representa la pestaña de "Entrada"
class InputTab : public wxPanel {
public:
    InputTab(wxWindow* parent);

    // Método público para que el Frame principal pueda obtener la configuración de la GUI
    ProgramOptions GetProgramOptions();
    void SetStartButtonState(bool enabled);

private:
    // Manejadores de eventos internos de la pestaña
    void OnAddFiles(wxCommandEvent& event);
    void OnInputChanged(wxEvent& event);
    void UpdateCommandPreview();
    void OnStart(wxCommandEvent& event);

    // Controles de la pestaña
    wxFilePickerCtrl *m_darkFilePicker, *m_satFilePicker;
    wxTextCtrl *m_darkValueText, *m_satValueText, *m_commandPreviewText;
    wxButton* m_startButton;
    wxListBox* m_fileListBox;
    wxArrayString m_inputFiles;
};