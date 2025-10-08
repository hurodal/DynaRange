// File: src/gui/DynaRangeFrame.hpp
/**
 * @file gui/DynaRangeFrame.hpp
 * @brief Main frame of the DynaRange GUI application (The View).
 */
#pragma once

#include "generated/DynaRangeBase.h"
#include "GuiPresenter.hpp"
#include "../core/arguments/ArgumentsOptions.hpp"
#include "../graphics/Constants.hpp"
#include <wx/dnd.h>
#include <wx/timer.h>
#include <wx/notebook.h>
#include <wx/statbmp.h>
#include <wx/webview.h> // Added for wxWebView support
#include <string>

// Forward declarations
class ChartController;
class FileDropTarget;
class InputController;
class LogController;
class ResultsController;
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
    DynaRange::Graphics::Constants::PlotOutputFormat GetPlotFormat() const;
    std::vector<double> GetChartCoords() const;
    std::vector<std::string> GetInputFiles() const;
    int GetChartPatchesM() const;
    int GetChartPatchesN() const;
    std::string GetPrintPatchesFilename() const;
    RawChannelSelection GetRawChannelSelection() const;

protected:
    // --- Event Handlers ---
    void OnAddFilesClick(wxCommandEvent& event);
    void OnChartChartPatchChanged(wxCommandEvent& event);
    void OnChartColorSliderChanged(wxScrollEvent& event);
    void OnChartCreateClick(wxCommandEvent& event);
    void OnChartInputChanged(wxCommandEvent& event);
    void OnChartPreviewClick(wxCommandEvent& event);
    void OnClearDarkFile(wxCommandEvent& event);
    void OnClearSaturationFile(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnDebugPatchesCheckBoxChanged(wxCommandEvent& event);
    void OnDrNormSliderChanged(wxScrollEvent& event);
    void OnExecuteClick(wxCommandEvent& event);
    void OnGaugeTimer(wxTimerEvent& event);
    void OnGridCellClick(wxGridEvent& event);
    void OnInputChanged(wxEvent& event);
    void OnInputChartPatchChanged(wxCommandEvent& event);
    void OnListBoxKeyDown(wxKeyEvent& event);
    void OnListBoxSelectionChanged(wxCommandEvent& event);
    void OnNotebookPageChanged(wxNotebookEvent& event);
    void OnPatchRatioSliderChanged(wxScrollEvent& event);
    void OnRemoveFilesClick(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnSnrSliderChanged(wxScrollEvent& event);
    void OnSplitterSashChanged(wxSplitterEvent& event);
    void OnSplitterSashDClick(wxSplitterEvent& event);
    void OnWorkerCompleted(wxCommandEvent& event);
    void OnWorkerUpdate(wxThreadEvent& event);
    /**
     * @brief Pointer to the web view component used to display plots.
     * @details This is created manually in the constructor and placed inside
     * m_webViewPlaceholderPanel.
     */
    wxWebView* m_resultsWebView;
    void OnWebViewError(wxWebViewEvent& event);

private:
    /**
     * @brief Bitmap control to display the PNG preview of the chart in the Chart tab.
     */
    wxWebView* m_chartPreviewWebView = nullptr; 

    // --- Member variables ---
    std::unique_ptr<GuiPresenter> m_presenter;
    wxTimer* m_gaugeTimer;
    FileDropTarget* m_dropTarget;

    // Added a flag to prevent infinite event loops during sync.
    bool m_isUpdatingPatches = false;

    // --- Controller Class Members ---
    std::unique_ptr<InputController> m_inputController;
    std::unique_ptr<LogController> m_logController;
    std::unique_ptr<ResultsController> m_resultsController;
    std::unique_ptr<ChartController> m_chartController;

    // Grant controllers access to protected UI members.
    friend class InputController;
    friend class ResultsController;
    friend class ChartController;
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

