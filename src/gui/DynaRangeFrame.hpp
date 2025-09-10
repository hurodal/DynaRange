/**
 * @file DynaRangeFrame.hpp
 * @brief Main frame of the DynaRange GUI application.
 * @author Juanma Font
 * @date 2025-09-10
 */
#pragma once

#include "DynaRangeBase.h"
#include "../core/Arguments.hpp"
#include <wx/arrstr.h>
#include <wx/dnd.h>
#include <string>

// Custom event declarations for the worker thread
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);

// ADDED: Forward declaration to solve the 'FileDropTarget does not name a type' error.
// This tells the compiler that FileDropTarget is a class before it is fully defined.
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

    /**
     * @brief Adds a list of files, received from a drop operation, to the input list.
     * @param filenames An array of full file paths.
     */
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

private:
    // Logic functions
    void UpdateCommandPreview();
    void ClearLog();
    void AppendLog(const wxString& text);
    void LoadResults(const ProgramOptions& opts);
    void LoadGraphImage(const wxString& path_or_raw_name);
    ProgramOptions GetProgramOptions();
    void SetExecuteButtonState(bool enabled);

    /**
     * @brief Checks if a given file is a RAW format supported by LibRaw.
     * @param filePath The full path to the file.
     * @return true if the file is a supported RAW, false otherwise.
     */
    bool IsSupportedRawFile(const wxString& filePath);

    /**
     * @brief Filters a list of file paths, adding only valid RAWs to the input list.
     * @param paths An array of file paths to process.
     */
    void AddRawFilesToList(const wxArrayString& paths);

    // Member variables
    ProgramOptions m_lastRunOptions;
    wxArrayString m_inputFiles;
    std::string m_summaryPlotPath;
    FileDropTarget* m_dropTarget;
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