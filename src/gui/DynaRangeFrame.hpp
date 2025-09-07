#pragma once

#include "DynaRangeBase.h"
#include "../core/Arguments.hpp" // Para ProgramOptions

// Declaración de los eventos personalizados para el hilo de trabajo
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);

class DynaRangeFrame : public MyFrameBase
{
public:
    DynaRangeFrame(wxWindow* parent);
    ~DynaRangeFrame();

private:
    // --- Manejadores de eventos de la GUI ---
    void OnExecuteClick(wxCommandEvent& event);
    void OnAddFilesClick(wxCommandEvent& event);
    void OnInputChanged(wxEvent& event);
    void OnGridCellClick(wxGridEvent& event);

    // --- Manejadores de eventos del hilo de trabajo ---
    void OnWorkerUpdate(wxThreadEvent& event);
    void OnWorkerCompleted(wxThreadEvent& event);

    // --- Funciones de lógica migradas ---
    void UpdateCommandPreview();
    void ClearLog();
    void AppendLog(const wxString& text);
    void LoadResults(const ProgramOptions& opts);
    void LoadGraphImage(const wxString& rawFilename);
    ProgramOptions GetProgramOptions();
    void SetExecuteButtonState(bool enabled);

    // --- Variables miembro migradas ---
    ProgramOptions m_lastRunOptions;
    bool m_success;
    wxArrayString m_inputFiles;
};