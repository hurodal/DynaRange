// File: gui/DynaRangeFrame.hpp
/**
 * @file gui/DynaRangeFrame.hpp
 * @brief Main frame of the DynaRange GUI application (The View).
 * @author Juanma Font
 * @date 2025-09-10
 */
#pragma once

#include "DynaRangeBase.h"
#include "GuiPresenter.hpp" // Include the presenter
#include <wx/arrstr.h>
#include <wx/dnd.h>
#include <wx/timer.h>
#include <string>
#include <memory> // For std::unique_ptr

// Custom event declarations for thread communication
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxCommandEvent);

class FileDropTarget;

/**
 * @class DynaRangeFrame
 * @brief Implements the application's main window (the View).
 * @details This class is responsible only for UI elements and events.
 * It delegates all application logic to a GuiPresenter instance.
 */
class DynaRangeFrame : public MyFrameBase
{
public:
    DynaRangeFrame(wxWindow* parent);
    ~DynaRangeFrame();

    // --- Methods called by external classes (e.g., FileDropTarget) ---
    void AddDroppedFiles(const wxArrayString& filenames);
    // --- Methods called by the Presenter to update the View ---
    void UpdateInputFileList(const std::vector<std::string>& files);
    void UpdateCommandPreview(const std::string& command);
    void DisplayResults(const std::string& csv_path);
    void ShowError(const wxString& title, const wxString& message);
    void SetUiState(bool is_processing);
    void PostLogUpdate(const std::string& text);
    void PostAnalysisComplete();
    void LoadGraphImage(const std::string& image_path);

    // --- Getters for the Presenter to get data from the View ---
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

protected:
    // Event Handlers that delegate to the Presenter
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
    //void OnCliPaneChanged(wxCollapsiblePaneEvent& event);
    
    // Worker thread event handlers
    void OnWorkerUpdate(wxThreadEvent& event);
    void OnWorkerCompleted(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    
private:
    // Private UI helper functions
    void ClearLog();
    void AppendLog(const wxString& text);
    bool IsSupportedRawFile(const wxString& filePath);
    void LoadLogoImage();
    void PerformFileRemoval();

    // Member variables
    std::unique_ptr<GuiPresenter> m_presenter;
    FileDropTarget* m_dropTarget;
    wxTimer* m_gaugeTimer;
};

class FileDropTarget : public wxFileDropTarget
{
public:
    FileDropTarget(DynaRangeFrame* owner) : m_owner(owner) {}
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;
private:
    DynaRangeFrame* m_owner;
};