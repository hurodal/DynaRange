// dynRangeGui.cpp
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/filepicker.h>
#include <wx/valnum.h>
#include <wx/grid.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>
#include <wx/listbox.h>
#include <wx/intl.h>

#include <thread>
#include <sstream>
#include <fstream>
#include <string>

#include "core/arguments.hpp"
#include "core/engine.hpp"
#include "core/gui/InputTab.hpp"
#include "core/gui/LogTab.hpp"
#include "core/gui/ResultsTab.hpp"
#include "core/gui/EventIDs.hpp"

#include <libintl.h>
#include <locale.h>

const int ID_START_BUTTON_FROM_TAB = 2001;

wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);

class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);
private:
    void OnStart(wxCommandEvent& event);
    void OnWorkerCompleted(wxThreadEvent& event);
    wxNotebook* m_notebook;
    InputTab* m_inputTab;
    LogTab* m_logTab;
    ResultsTab* m_resultsTab;
    std::string m_logOutput;
    ProgramOptions m_lastRunOptions;
    bool m_success;
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    // Forzamos un locale estándar y robusto que soporta UTF-8.
    // Esto evita problemas con configuraciones de sistema inconsistentes.
    setlocale(LC_ALL, "C.UTF-8");

    bindtextdomain("dynrange", "locale");
    textdomain("dynrange");

    MyFrame *frame = new MyFrame(_("Dynamic Range Calculator"));
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(700, 800)) {
    
    wxPanel* mainPanel = new wxPanel(this);
    m_notebook = new wxNotebook(mainPanel, wxID_ANY);

    m_inputTab = new InputTab(m_notebook);
    m_logTab = new LogTab(m_notebook);
    m_resultsTab = new ResultsTab(m_notebook);

    m_notebook->AddPage(m_inputTab, _("Input"));
    m_notebook->AddPage(m_logTab, _("Log"));
    m_notebook->AddPage(m_resultsTab, _("Results"));
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
    mainPanel->SetSizerAndFit(mainSizer);
    this->Layout();

    Bind(wxEVT_BUTTON, &MyFrame::OnStart, this, ID_START_BUTTON_FROM_TAB);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &MyFrame::OnWorkerCompleted, this);
}

void MyFrame::OnStart(wxCommandEvent& event) {
    m_lastRunOptions = m_inputTab->GetProgramOptions();
    if (m_lastRunOptions.input_files.empty()) {
        wxMessageBox(_("Please select at least one input RAW file."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }
    
    m_inputTab->SetStartButtonState(false);
    m_notebook->SetSelection(1);
    m_logTab->ClearLog();
    m_logTab->AppendLog(_("Execution started...\n---\n"));

    std::thread worker_thread([this]() {
        std::stringstream log_stream;
        m_success = run_dynamic_range_analysis(m_lastRunOptions, log_stream);
        m_logOutput = log_stream.str();
        wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED));
    });

    worker_thread.detach();
}

void MyFrame::OnWorkerCompleted(wxThreadEvent& event) {
    m_logTab->AppendLog(m_logOutput);
    m_inputTab->SetStartButtonState(true);

    if (!m_success) {
        m_logTab->AppendLog(_("\n---\nExecution failed."));
        wxMessageBox(_("An error occurred during processing. Please check the log tab for details."), 
                     _("Error"), wxOK | wxICON_ERROR, this);
    } else {
        m_logTab->AppendLog(_("\n---\nExecution finished successfully."));
        m_notebook->SetSelection(2);
        m_resultsTab->LoadResults(m_lastRunOptions);
    }
}