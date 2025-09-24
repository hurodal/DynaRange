// File: gui/DynaRangeFrame.hpp
/**
 * @file gui/DynaRangeFrame.hpp
 * @brief Main frame of the DynaRange GUI application (The View).
 */
#pragma once

#include "DynaRangeBase.h"
#include "GuiPresenter.hpp"
#include <wx/dnd.h>
#include <wx/timer.h>
#include <wx/notebook.h> // Required for wxNotebookEvent
#include <string>
#include <memory>
#include <vector>

// Forward declarations
class InputController;
class LogController;
class ResultsController;
class FileDropTarget;
class wxSplitterEvent;

// Custom event declarations
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxCommandEvent);

class DynaRangeFrame : public MyFrameBase
{
public:
    DynaRangeFrame(wxWindow* parent);
    ~DynaRangeFrame();

    // --- Methods called by the Presenter to update the View ---
    void UpdateInputFileList(const std::vector<std::string>& files);
    void UpdateCommandPreview(const std::string& command);
    void DisplayResults(const std::string& csv_path);
    void ShowError(const wxString& title, const wxString& message);
    void SetUiState(bool is_processing);
    void PostLogUpdate(const std::string& text);
    void PostAnalysisComplete();
    void LoadGraphImage(const std::string& image_path);

    // --- Getters that delegate to InputController ---
    std::string GetDarkFilePath() const;
    std::string GetSaturationFilePath() const;
    double GetDarkValue() const;
    double GetSaturationValue() const;
    double GetPatchRatio() const;
    std::string GetOutputFilePath() const;
    double GetSnrThreshold() const;
    double GetDrNormalization() const;
    int GetPolyOrder() const;
    int GetPlotMode() const;
    std::vector<std::string> GetInputFiles() const;

protected:
    // --- Event Handlers ---
    void OnExecuteClick(wxCommandEvent& event);
    void OnAddFilesClick(wxCommandEvent& event);
    void OnGridCellClick(wxGridEvent& event);
    void OnInputChanged(wxEvent& event);
    void OnGaugeTimer(wxTimerEvent& event);
    void OnPatchRatioSliderChanged(wxScrollEvent& event);
    void OnRemoveFilesClick(wxCommandEvent& event);
    void OnListBoxSelectionChanged(wxCommandEvent& event);
    void OnListBoxKeyDown(wxKeyEvent& event);
    void OnSnrSliderChanged(wxScrollEvent& event);
    void OnDrNormSliderChanged(wxScrollEvent& event);
    void OnWorkerUpdate(wxThreadEvent& event);
    void OnWorkerCompleted(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnSplitterSashDClick(wxSplitterEvent& event);
    void OnSplitterSashChanged(wxSplitterEvent& event);
    void OnNotebookPageChanged(wxNotebookEvent& event);

private:
    // --- Member variables ---
    std::unique_ptr<GuiPresenter> m_presenter;
    wxTimer* m_gaugeTimer;
    FileDropTarget* m_dropTarget;
    // --- Controller Class Members ---
    std::unique_ptr<InputController> m_inputController;
    std::unique_ptr<LogController> m_logController;
    std::unique_ptr<ResultsController> m_resultsController;

    friend class InputController;
    friend class ResultsController;
    friend class FileDropTarget;
};

class FileDropTarget : public wxFileDropTarget
{
public:
    FileDropTarget(DynaRangeFrame* owner) : m_owner(owner) {}
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;
private:
    DynaRangeFrame* m_owner;
};