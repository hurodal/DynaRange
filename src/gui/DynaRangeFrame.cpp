#include "DynaRangeFrame.hpp"
#include "../core/engine.hpp"
#include "../core/functions.hpp"

#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <thread>
#include <ostream>
#include <streambuf>
#include <fstream>
#include <sstream>

// --- LÓGICA DE LOGGING Y EVENTOS ---
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);

class WxLogStreambuf : public std::streambuf {
public:
    WxLogStreambuf(wxEvtHandler* target) : m_target(target) {}
protected:
    virtual int sync() override {
        if (!m_buffer.empty()) {
            wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_WORKER_UPDATE);
            event->SetString(m_buffer);
            wxQueueEvent(m_target, event);
            m_buffer.clear();
        }
        return 0;
    }
    virtual int overflow(int c) override {
        if (c != EOF) {
            m_buffer += static_cast<char>(c);
            if (c == '\n') { sync(); }
        }
        return c;
    }
private:
    wxEvtHandler* m_target;
    std::string m_buffer;
};

// --- IMPLEMENTACIÓN DE LA VENTANA ---

DynaRangeFrame::DynaRangeFrame(wxWindow* parent) : MyFrameBase(parent)
{
    // --- Conexiones de Eventos (Binding) ---
    m_executeButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    
    m_darkFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);

    UpdateCommandPreview();
}

DynaRangeFrame::~DynaRangeFrame()
{
    // Desconectamos los eventos para evitar problemas al cerrar
    m_executeButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    Unbind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Unbind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    m_darkFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
}

void DynaRangeFrame::OnExecuteClick(wxCommandEvent& event)
{
    m_lastRunOptions = GetProgramOptions();
    if (m_lastRunOptions.input_files.empty()) {
        wxMessageBox(_("Please select at least one input RAW file."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    SetExecuteButtonState(false);
    m_mainNotebook->SetSelection(1);
    ClearLog();

    std::thread worker_thread([this]() {
        WxLogStreambuf log_streambuf(this);
        std::ostream log_stream(&log_streambuf);
        
        ProgramOptions opts = m_lastRunOptions;

        if (!opts.dark_file_path.empty()) {
            auto dark_val_opt = process_dark_frame(opts.dark_file_path, log_stream);
            if (!dark_val_opt) { m_success = false; wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED)); return; }
            opts.dark_value = *dark_val_opt;
        }

        if (!opts.sat_file_path.empty()) {
            auto sat_val_opt = process_saturation_frame(opts.sat_file_path, log_stream);
            if (!sat_val_opt) { m_success = false; wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED)); return; }
            opts.saturation_value = *sat_val_opt;
        }

        if (!prepare_and_sort_files(opts, log_stream)) {
            m_success = false; wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED)); return;
        }

        m_success = run_dynamic_range_analysis(opts, log_stream);
        wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED));
    });

    worker_thread.detach();
}

void DynaRangeFrame::OnWorkerUpdate(wxThreadEvent& event) {
    AppendLog(event.GetString());
}

void DynaRangeFrame::OnWorkerCompleted(wxThreadEvent& event) {
    SetExecuteButtonState(true);
    if (!m_success) {
        AppendLog(_("\n---\nExecution failed. Please check the log for details."));
        wxMessageBox(_("An error occurred during processing. Please check the log tab for details."), _("Error"), wxOK | wxICON_ERROR, this);
    } else {
        AppendLog(_("\n---\nExecution finished successfully."));
        m_mainNotebook->SetSelection(2);
        LoadResults(m_lastRunOptions);
    }
}

void DynaRangeFrame::OnAddFilesClick(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, _("Select RAW files"), "", "", "RAW files (*.dng;*.cr2;*.nef)|*.dng;*.cr2;*.nef", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (openFileDialog.ShowModal() == wxID_CANCEL) return;
    openFileDialog.GetPaths(m_inputFiles);
    m_rawFileslistBox->Set(m_inputFiles);
    UpdateCommandPreview();
}

void DynaRangeFrame::OnInputChanged(wxEvent& event) {
    UpdateCommandPreview();
}

ProgramOptions DynaRangeFrame::GetProgramOptions() {
    ProgramOptions opts;
    opts.dark_file_path = std::string(m_darkFilePicker->GetPath().mb_str());
    opts.sat_file_path = std::string(m_saturationFilePicker->GetPath().mb_str());
    m_darkValueTextCtrl->GetValue().ToDouble(&opts.dark_value);
    m_saturationValueTextCtrl->GetValue().ToDouble(&opts.saturation_value);
    for (const wxString& file : m_inputFiles) {
        opts.input_files.push_back(std::string(file.mb_str()));
    }
    opts.output_filename = "DR_results.csv";
    return opts;
}

void DynaRangeFrame::UpdateCommandPreview() {
    wxString command = "./dynRange";
    if (!m_darkFilePicker->GetPath().IsEmpty()) { command += " --dark-file \"" + m_darkFilePicker->GetPath() + "\""; }
    else { command += " --dark-value " + m_darkValueTextCtrl->GetValue(); }
    if (!m_saturationFilePicker->GetPath().IsEmpty()) { command += " --sat-file \"" + m_saturationFilePicker->GetPath() + "\""; }
    else { command += " --sat-value " + m_saturationValueTextCtrl->GetValue(); }
    command += " -f";
    for (const wxString& file : m_inputFiles) { command += " \"" + file + "\""; }
    m_equivalentCliTextCtrl->ChangeValue(command);
}

void DynaRangeFrame::SetExecuteButtonState(bool enabled) {
    m_executeButton->Enable(enabled);
}

void DynaRangeFrame::ClearLog() {
    m_logOutputTextCtrl->Clear();
}

void DynaRangeFrame::AppendLog(const wxString& text) {
    m_logOutputTextCtrl->AppendText(text);
}

void DynaRangeFrame::LoadResults(const ProgramOptions& opts) {
    std::ifstream file(opts.output_filename);
    if (!file) { return; }
    if (m_cvsGrid->GetNumberRows() > 0) m_cvsGrid->DeleteRows(0, m_cvsGrid->GetNumberRows());
    if (m_cvsGrid->GetNumberCols() > 0) m_cvsGrid->DeleteCols(0, m_cvsGrid->GetNumberCols());
    std::string line;
    int row = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        int col = 0;
        m_cvsGrid->AppendRows(1);
        while (std::getline(ss, cell, ',')) {
            if (col >= m_cvsGrid->GetNumberCols()) { m_cvsGrid->AppendCols(1); }
            m_cvsGrid->SetCellValue(row, col, cell);
            col++;
        }
        row++;
    }
    m_cvsGrid->AutoSize();
    this->Layout();
}