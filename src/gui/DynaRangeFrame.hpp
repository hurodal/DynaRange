#pragma once

#include "DynaRangeBase.h"
#include "../core/Arguments.hpp" // For ProgramOptions
#include <wx/arrstr.h>          // For wxArrayString
#include <string>               // Required for std::string

// Custom event declarations for the worker thread
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);

class DynaRangeFrame : public MyFrameBase
{
public:
    DynaRangeFrame(wxWindow* parent);
    ~DynaRangeFrame();

protected:
    // Event Handlers
    // **FIXED**: Removed 'override' as the base class does not use virtual event handlers.
    void OnExecuteClick(wxCommandEvent& event);
    void OnAddFilesClick(wxCommandEvent& event);
    void OnGridCellClick(wxGridEvent& event);

    // Worker thread event handlers
    void OnWorkerUpdate(wxThreadEvent& event);
    void OnWorkerCompleted(wxThreadEvent& event);
    
    // Other handlers
    void OnInputChanged(wxEvent& event);

private:
    // Logic functions
    void UpdateCommandPreview();
    void ClearLog();
    void AppendLog(const wxString& text);
    void LoadResults(const ProgramOptions& opts);
    void LoadGraphImage(const wxString& rawFilename);
    ProgramOptions GetProgramOptions();
    void SetExecuteButtonState(bool enabled);

    // Member variables
    ProgramOptions m_lastRunOptions;
    wxArrayString m_inputFiles;
    std::string m_summaryPlotPath; 
};