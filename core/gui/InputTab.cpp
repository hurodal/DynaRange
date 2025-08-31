// core/gui/InputTab.cpp
#include "InputTab.hpp"
#include "EventIDs.hpp"
#include <wx/valnum.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/intl.h>
#include <clocale>

#include "../../core/functions.hpp"

InputTab::InputTab(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    // Sizer principal que organiza toda la pestaña verticalmente
    wxBoxSizer* mainVBox = new wxBoxSizer(wxVERTICAL);

    // --- NUEVO LAYOUT: Sizer horizontal para la primera fila de controles ---
    wxBoxSizer* topRowSizer = new wxBoxSizer(wxHORIZONTAL);

    // --- Sección Dark Frame (Columna 1) ---
    wxStaticBoxSizer* darkSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Dark Frame"));
    // Usamos un FlexGridSizer para alinear etiquetas y controles en una tabla
    wxFlexGridSizer* darkGridSizer = new wxFlexGridSizer(2, 5, 5); // 2 columnas, con espacios de 5px
    darkGridSizer->AddGrowableCol(1, 1); // Hace que la segunda columna (controles) crezca

    wxFloatingPointValidator<double> val(2);
    m_darkFilePicker = new wxFilePickerCtrl(this, wxID_ANY, "", "Select Dark Frame file", "*.*", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
    m_darkValueText = new wxTextCtrl(this, wxID_ANY, "256.0", wxDefaultPosition, wxDefaultSize, 0, val);
    
    darkGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Dark File:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    darkGridSizer->Add(m_darkFilePicker, 1, wxEXPAND | wxALL, 5);
    darkGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Dark Value:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    darkGridSizer->Add(m_darkValueText, 1, wxEXPAND | wxALL, 5);
    darkSizer->Add(darkGridSizer, 1, wxEXPAND);
    
    topRowSizer->Add(darkSizer, 1, wxEXPAND | wxALL, 5); // Añadir a la fila, proporción 1 (ocupa la mitad)


    // --- Sección Saturation (Columna 2) ---
    wxStaticBoxSizer* satSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Saturation"));
    wxFlexGridSizer* satGridSizer = new wxFlexGridSizer(2, 5, 5);
    satGridSizer->AddGrowableCol(1, 1);

    m_satFilePicker = new wxFilePickerCtrl(this, wxID_ANY, "", "Select Saturation file", "*.*", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
    m_satValueText = new wxTextCtrl(this, wxID_ANY, "4095.0", wxDefaultPosition, wxDefaultSize, 0, val);
    
    satGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Saturation File:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    satGridSizer->Add(m_satFilePicker, 1, wxEXPAND | wxALL, 5);
    satGridSizer->Add(new wxStaticText(this, wxID_ANY, _("Saturation Value:")), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    satGridSizer->Add(m_satValueText, 1, wxEXPAND | wxALL, 5);
    satSizer->Add(satGridSizer, 1, wxEXPAND);

    topRowSizer->Add(satSizer, 1, wxEXPAND | wxALL, 5); // Añadir a la fila, proporción 1 (ocupa la otra mitad)

    // --- Controles restantes (debajo de las dos columnas) ---
    m_fileListBox = new wxListBox(this, wxID_ANY);
    wxButton* addFilesButton = new wxButton(this, wxID_ANY, _("Add RAW Files..."));
    wxStaticBoxSizer* filesSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Input RAW Files"));
    filesSizer->Add(m_fileListBox, 1, wxEXPAND | wxALL, 5);
    filesSizer->Add(addFilesButton, 0, wxALIGN_CENTER | wxALL, 5);

    m_commandPreviewText = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    wxStaticBoxSizer* cmdSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Equivalent CLI Command"));
    cmdSizer->Add(m_commandPreviewText, 1, wxEXPAND | wxALL, 5);

    m_startButton = new wxButton(this, ID_START_BUTTON_FROM_TAB, _("Execute"));
    
    // --- Montaje final del Sizer principal ---
    mainVBox->Add(topRowSizer, 0, wxEXPAND);
    mainVBox->Add(filesSizer, 1, wxEXPAND | wxALL, 5);
    mainVBox->Add(cmdSizer, 0, wxEXPAND | wxALL, 5);
    mainVBox->Add(m_startButton, 0, wxALIGN_CENTER | wxALL, 10);
    
    this->SetSizerAndFit(mainVBox);
    
    // --- Conexión de eventos (sin cambios) ---
    Bind(wxEVT_TEXT, &InputTab::OnInputChanged, this);
    Bind(wxEVT_FILEPICKER_CHANGED, &InputTab::OnInputChanged, this);
    Bind(wxEVT_BUTTON, &InputTab::OnStart, this, ID_START_BUTTON_FROM_TAB);
    Bind(wxEVT_BUTTON, &InputTab::OnAddFiles, this, addFilesButton->GetId());
    
    UpdateCommandPreview();
}

void InputTab::OnAddFiles(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, _("Select RAW files"), "", "", "RAW files (*.dng;*.cr2;*.nef)|*.dng;*.cr2;*.nef", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (openFileDialog.ShowModal() == wxID_CANCEL) return;
    openFileDialog.GetPaths(m_inputFiles);
    m_fileListBox->Set(m_inputFiles);
    UpdateCommandPreview();
}

void InputTab::OnInputChanged(wxEvent& event) { UpdateCommandPreview(); }

void InputTab::UpdateCommandPreview() {
    wxString command = "./dynRange";
    if (!m_darkFilePicker->GetPath().IsEmpty()) { command += " --dark-file \"" + m_darkFilePicker->GetPath() + "\""; }
    else { command += " --dark-value " + m_darkValueText->GetValue(); }
    if (!m_satFilePicker->GetPath().IsEmpty()) { command += " --sat-file \"" + m_satFilePicker->GetPath() + "\""; }
    else { command += " --sat-value " + m_satValueText->GetValue(); }
    command += " -f";
    for (const wxString& file : m_inputFiles) { command += " \"" + file + "\""; }
    m_commandPreviewText->ChangeValue(command);
}

void InputTab::OnStart(wxCommandEvent& event) { wxPostEvent(GetParent()->GetParent(), event); }

ProgramOptions InputTab::GetProgramOptions() {
    ProgramOptions opts;
    
    // This function now only collects information, it does NOT process it.
    
    // Collects the paths of the selected files
    opts.dark_file_path = std::string(m_darkFilePicker->GetPath().mb_str());
    opts.sat_file_path = std::string(m_satFilePicker->GetPath().mb_str());

    // Temporarily switch locale to safely parse numbers with a decimal point
    char* current_locale = setlocale(LC_NUMERIC, nullptr);
    setlocale(LC_NUMERIC, "C");
    m_darkValueText->GetValue().ToDouble(&opts.dark_value);
    m_satValueText->GetValue().ToDouble(&opts.saturation_value);
    setlocale(LC_NUMERIC, current_locale);
    
    // Collects the list of input files
    for (const wxString& file : m_inputFiles) {
        opts.input_files.push_back(std::string(file.mb_str()));
    }
    
    opts.output_filename = "DR_results.csv";
    
    return opts;
}
void InputTab::SetStartButtonState(bool enabled) {
    m_startButton->Enable(enabled);
}