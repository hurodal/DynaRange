// dynRangeGui.cpp
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/intl.h>
#include <thread>
#include <sstream>
#include <string>
#include <fstream>
#include <filesystem>
#include <streambuf> // Required for the custom streambuf
#include <ostream>   // Required for std::ostream

#include "core/arguments.hpp"
#include "core/engine.hpp"
#include "core/gui/InputTab.hpp"
#include "core/gui/LogTab.hpp"
#include "core/gui/ResultsTab.hpp"
#include "core/gui/EventIDs.hpp"
#include "core/functions.hpp"

// Use the standard C initialization, which is more robust for our core library
#include <libintl.h>
#include <locale.h>

// --- START: LOGIC FOR REAL-TIME LOGGING ---

// 1. Define two custom event types
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);    // To send a log line
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent); // To notify completion

/**
 * @class WxLogStreambuf
 * @brief A custom streambuf that redirects the output of a std::ostream
 * to a wxWidgets event handler.
 */
class WxLogStreambuf : public std::streambuf {
public:
    WxLogStreambuf(wxEvtHandler* target) : m_target(target) {}

protected:
    // This is called when the buffer needs to be flushed (e.g., by std::endl)
    virtual int sync() override {
        if (!m_buffer.empty()) {
            // Create a new update event
            wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_WORKER_UPDATE);
            // Attach the current log line as a string
            event->SetString(m_buffer);
            // Safely post the event to the main thread's event queue
            wxQueueEvent(m_target, event);
            // Clear the buffer
            m_buffer.clear();
        }
        return 0;
    }

    // This is called when a character is written to the stream
    virtual int overflow(int c) override {
        if (c != EOF) {
            m_buffer += static_cast<char>(c);
            // If it's a newline character, flush the buffer immediately
            if (c == '\n') {
                sync();
            }
        }
        return c;
    }

private:
    wxEvtHandler* m_target; // The window to send events to
    std::string m_buffer;   // A buffer to hold the current line
};
// --- END: LOGIC FOR REAL-TIME LOGGING ---


// --- GUI Class Declarations ---
class MyApp;
class MyFrame;

// --- Main Frame Class ---
class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);
private:
    void OnStart(wxCommandEvent& event);
    void OnWorkerUpdate(wxThreadEvent& event);   // NEW: Handler for log updates
    void OnWorkerCompleted(wxThreadEvent& event);

    wxNotebook* m_notebook;
    InputTab* m_inputTab;
    LogTab* m_logTab;
    ResultsTab* m_resultsTab;
    
    ProgramOptions m_lastRunOptions;
    bool m_success;
};

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

    // Connect the events
    Bind(wxEVT_BUTTON, &MyFrame::OnStart, this, ID_START_BUTTON_FROM_TAB);
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &MyFrame::OnWorkerUpdate, this); // Connect the new log update event
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &MyFrame::OnWorkerCompleted, this);
}

void MyFrame::OnStart(wxCommandEvent& event) {
    // 1. The GUI gathers the "recipe" from the input tab.
    m_lastRunOptions = m_inputTab->GetProgramOptions();
    if (m_lastRunOptions.input_files.empty()) {
        wxMessageBox(_("Please select at least one input RAW file."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }
    
    m_inputTab->SetStartButtonState(false);
    m_notebook->SetSelection(1);
    m_logTab->ClearLog();

    // 2. The worker thread is launched, which will handle ALL processing.
    std::thread worker_thread([this]() {
        setlocale(LC_ALL, "C.UTF-8");

        WxLogStreambuf log_streambuf(this);
        std::ostream log_stream(&log_streambuf);
        
        ProgramOptions opts = m_lastRunOptions; // Create a local copy for the thread

        // --- START OF PREPARATION LOGIC IN THE THREAD ---
        
        // Step A: Process dark frame file, if one was provided.
        if (!opts.dark_file_path.empty()) {
            auto dark_val_opt = process_dark_frame(opts.dark_file_path, log_stream);
            if (!dark_val_opt) {
                m_success = false;
                wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED));
                return; // End thread on failure
            }
            opts.dark_value = *dark_val_opt; // Overwrite the value from the text box
        }

        // Step B: Process saturation file, if one was provided.
        if (!opts.sat_file_path.empty()) {
            auto sat_val_opt = process_saturation_frame(opts.sat_file_path, log_stream);
            if (!sat_val_opt) {
                m_success = false;
                wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED));
                return; // End thread on failure
            }
            opts.saturation_value = *sat_val_opt; // Overwrite the value from the text box
        }

        // Step C: Sort the input files
        if (!prepare_and_sort_files(opts, log_stream)) {
            m_success = false;
            wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED));
            return; // End thread on failure
        }
        // --- END OF PREPARATION LOGIC ---

        // 3. With the data now fully prepared, call the main engine.
        m_success = run_dynamic_range_analysis(opts, log_stream);
        
        // 4. Notify the main window that the work is finished.
        wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED));
    });

    worker_thread.detach();
}

// New event handler that executes on the main thread every time a log line arrives
void MyFrame::OnWorkerUpdate(wxThreadEvent& event) {
    m_logTab->AppendLog(event.GetString());
}

// Handler that executes once when the thread is finished
void MyFrame::OnWorkerCompleted(wxThreadEvent& event) {
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


// --- Main Application Class ---
class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    // Set a standard, robust locale that supports UTF-8.
    setlocale(LC_ALL, "C.UTF-8");
    bindtextdomain("dynrange", "locale");
    textdomain("dynrange");

    MyFrame *frame = new MyFrame(_("Dynamic Range Calculator"));
    frame->Show(true);
    return true;
}