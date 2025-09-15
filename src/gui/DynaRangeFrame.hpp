/**
 * @file gui/DynaRangeFrame.hpp
 * @brief Main frame of the DynaRange GUI application.
 * @author Juanma Font
 * @date 2025-09-10
 */
#pragma once

#include "DynaRangeBase.h"
#include "../core/arguments/Arguments.hpp"
#include <wx/arrstr.h>
#include <wx/dnd.h>
#include <wx/timer.h> // AÑADIDO: Cabecera para el wxTimer
#include <string>
#include <map>

// Custom event declarations for the worker thread
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);

// Forward declaration
class FileDropTarget;

/**
 * @class DynaRangeFrame
 * @brief Implements the application's main window, handling user interaction and events.
 */
class DynaRangeFrame : public MyFrameBase
{
public:
    DynaRangeFrame(wxWindow* parent);
    ~DynaRangeFrame();

    void AddDroppedFiles(const wxArrayString& filenames);

protected:
    // Event Handlers
    void OnExecuteClick(wxCommandEvent& event);
    void OnAddFilesClick(wxCommandEvent& event);
    void OnGridCellClick(wxGridEvent& event);
    void OnInputChanged(wxEvent& event);

    // Worker thread event handlers
    void OnWorkerUpdate(wxThreadEvent& event);
    void OnWorkerCompleted(wxThreadEvent& event);
    
    /**
     * @brief Event handler for the gauge animation timer.
     * @param event The timer event.
     */
    void OnGaugeTimer(wxTimerEvent& event); // Handler para el temporizador

private:
    // Logic functions
    void UpdateCommandPreview();
    void ClearLog();
    void AppendLog(const wxString& text);
    void LoadResults(const ProgramOptions& opts);
    void LoadGraphImage(const wxString& path_or_raw_name);
    ProgramOptions GetProgramOptions();
    void SetExecuteButtonState(bool enabled);
    bool IsSupportedRawFile(const wxString& filePath);
    void AddRawFilesToList(const wxArrayString& paths);

    /**
     * @brief Manages the UI state of the results panel.
     * @param processing true to enter 'processing' state, false to enter 'results' state.
     */
    void SetResultsPanelState(bool processing);

    // Member variables
    ProgramOptions m_lastRunOptions;
    wxArrayString m_inputFiles;
    std::string m_summaryPlotPath;
    std::map<std::string, std::string> m_individualPlotPaths;
    FileDropTarget* m_dropTarget;
    
    wxTimer* m_gaugeTimer; // Puntero al temporizador
};

/**
 * @class FileDropTarget
 * @brief A wxFileDropTarget implementation to handle files dragged onto the frame.
 *
 * This class has the single responsibility of receiving file drop events and
 * forwarding the file paths to the main application frame.
 */
class FileDropTarget : public wxFileDropTarget
{
public:
    /**
     * @brief Constructs the drop target.
     * @param owner A pointer to the DynaRangeFrame that will process the files.
     */
    FileDropTarget(DynaRangeFrame* owner) : m_owner(owner) {}

    /**
     * @brief Called by wxWidgets when files are dropped onto the associated window.
     * @param x The x-coordinate of the drop point.
     * @param y The y-coordinate of the drop point.
     * @param filenames An array of full paths for the dropped files.
     * @return true to indicate that the drop was successfully handled.
     */
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;

private:
    DynaRangeFrame* m_owner;
};