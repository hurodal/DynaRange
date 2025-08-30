// core/gui/InputTab.cpp
#include "InputTab.hpp"
#include "EventIDs.hpp"
#include <wx/valnum.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/intl.h>

extern const int ID_START_BUTTON_FROM_TAB;

InputTab::InputTab(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    wxFloatingPointValidator<double> val(2);

    m_darkFilePicker = new wxFilePickerCtrl(this, wxID_ANY, "", "Select Dark Frame file", "*.*", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
    m_darkValueText = new wxTextCtrl(this, wxID_ANY, "256.0", wxDefaultPosition, wxDefaultSize, 0, val);
    m_satFilePicker = new wxFilePickerCtrl(this, wxID_ANY, "", "Select Saturation file", "*.*", wxDefaultPosition, wxDefaultSize, wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
    m_satValueText = new wxTextCtrl(this, wxID_ANY, "4095.0", wxDefaultPosition, wxDefaultSize, 0, val);
    m_fileListBox = new wxListBox(this, wxID_ANY);
    wxButton* addFilesButton = new wxButton(this, wxID_ANY, _("Add RAW Files..."));
    m_commandPreviewText = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    m_startButton = new wxButton(this, ID_START_BUTTON_FROM_TAB, _("Execute"));

    wxStaticBoxSizer* darkSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Dark Frame"));
    darkSizer->Add(new wxStaticText(this, wxID_ANY, _("Dark File:")), 0, wxALL, 5);
    darkSizer->Add(m_darkFilePicker, 0, wxEXPAND | wxALL, 5);
    darkSizer->Add(new wxStaticText(this, wxID_ANY, _("Dark Value:")), 0, wxALL, 5);
    darkSizer->Add(m_darkValueText, 0, wxEXPAND | wxALL, 5);

    wxStaticBoxSizer* satSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Saturation"));
    satSizer->Add(new wxStaticText(this, wxID_ANY, _("Saturation File:")), 0, wxALL, 5);
    satSizer->Add(m_satFilePicker, 0, wxEXPAND | wxALL, 5);
    satSizer->Add(new wxStaticText(this, wxID_ANY, _("Saturation Value:")), 0, wxALL, 5);
    satSizer->Add(m_satValueText, 0, wxEXPAND | wxALL, 5);

    wxStaticBoxSizer* filesSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Input RAW Files"));
    filesSizer->Add(m_fileListBox, 1, wxEXPAND | wxALL, 5);
    filesSizer->Add(addFilesButton, 0, wxALIGN_CENTER | wxALL, 5);

    wxStaticBoxSizer* cmdSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Equivalent CLI Command"));
    cmdSizer->Add(m_commandPreviewText, 0, wxEXPAND | wxALL, 5);

    vbox->Add(darkSizer, 0, wxEXPAND | wxALL, 5);
    vbox->Add(satSizer, 0, wxEXPAND | wxALL, 5);
    vbox->Add(filesSizer, 1, wxEXPAND | wxALL, 5);
    vbox->Add(cmdSizer, 0, wxEXPAND | wxALL, 5);
    vbox->Add(m_startButton, 0, wxALIGN_CENTER | wxALL, 10);
    
    this->SetSizerAndFit(vbox);
    
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
    
    // --- INICIO DE LA CORRECCIÓN ---
    // Usamos ChangeValue para cambiar el texto sin generar un nuevo evento de cambio de texto.
    m_commandPreviewText->ChangeValue(command);
    // --- FIN DE LA CORRECCIÓN ---
}
void InputTab::OnStart(wxCommandEvent& event) { wxPostEvent(GetParent()->GetParent(), event); }
ProgramOptions InputTab::GetProgramOptions() {
    ProgramOptions opts;
    m_darkValueText->GetValue().ToDouble(&opts.dark_value);
    m_satValueText->GetValue().ToDouble(&opts.saturation_value);
    for (const wxString& file : m_inputFiles) { opts.input_files.push_back(std::string(file.mb_str())); }
    opts.output_filename = "DR_results.csv";
    return opts;
}
void InputTab::SetStartButtonState(bool enabled) { m_startButton->Enable(enabled); }