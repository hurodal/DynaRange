// core/gui/InputTab.cpp
#include "InputTab.hpp"
#include "EventIDs.hpp"
#include <wx/valnum.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/intl.h>
#include <clocale> // Required for setlocale

#include "../../core/functions.hpp"

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
    m_commandPreviewText->ChangeValue(command);
}
void InputTab::OnStart(wxCommandEvent& event) { wxPostEvent(GetParent()->GetParent(), event); }

ProgramOptions InputTab::GetProgramOptions() {
    ProgramOptions opts;
    
    // --- START OF THE FIX ---
    // Save the current numeric locale (e.g., "es_ES.UTF-8")
    char* current_locale = setlocale(LC_NUMERIC, nullptr);
    // Temporarily switch to the "C" locale so that decimal points '.' are recognized
    setlocale(LC_NUMERIC, "C");

    // Logic to get the black level value
    wxString dark_path = m_darkFilePicker->GetPath();
    if (!dark_path.IsEmpty()) {
        auto dark_val_opt = process_dark_frame(std::string(dark_path.mb_str()), std::cout);
        if(dark_val_opt) {
            opts.dark_value = *dark_val_opt;
        } else {
            wxMessageBox("Error processing the selected dark frame file.", "Error", wxOK | wxICON_ERROR);
            opts.dark_value = -1;
        }
    } else {
        m_darkValueText->GetValue().ToDouble(&opts.dark_value);
    }

    // Logic to get the saturation value
    wxString sat_path = m_satFilePicker->GetPath();
    if (!sat_path.IsEmpty()) {
        auto sat_val_opt = process_saturation_frame(std::string(sat_path.mb_str()), std::cout);
        if(sat_val_opt) {
            opts.saturation_value = *sat_val_opt;
        } else {
            wxMessageBox("Error processing the selected saturation file.", "Error", wxOK | wxICON_ERROR);
            opts.saturation_value = -1;
        }
    } else {
        m_satValueText->GetValue().ToDouble(&opts.saturation_value);
    }
    
    // Restore the original locale
    setlocale(LC_NUMERIC, current_locale);
    // --- END OF THE FIX ---
    
    opts.dark_file_path = std::string(dark_path.mb_str());
    opts.sat_file_path = std::string(sat_path.mb_str());

    for (const wxString& file : m_inputFiles) {
        opts.input_files.push_back(std::string(file.mb_str()));
    }
    
    opts.output_filename = "DR_results.csv";
    
    return opts;
}
void InputTab::SetStartButtonState(bool enabled) { m_startButton->Enable(enabled); }