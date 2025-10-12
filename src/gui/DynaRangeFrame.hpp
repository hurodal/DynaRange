// File: src/gui/DynaRangeFrame.hpp
/**
 * @file DynaRangeFrame.hpp
 * @brief Implementation of the DynaRange GUI's main frame (the View).
 */
#pragma once

#include "../core/arguments/ArgumentsOptions.hpp"
#include "../graphics/Constants.hpp"
#include "GuiPresenter.hpp"
#include "generated/DynaRangeBase.h"
#include <cairo.h>
#include <string>
#include <wx/bitmap.h>
#include <wx/dnd.h>
#include <wx/image.h> // Needed for wxImage
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/timer.h>

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

class DynaRangeFrame : public MyFrameBase {
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
    void DisplayImage(const wxImage& image);

    // --- Getters that delegate to InputController ---
    std::string GetDarkFilePath() const;
    std::string GetSaturationFilePath() const;
    double GetDarkValue() const;
    double GetSaturationValue() const;
    double GetPatchRatio() const;
    std::string GetOutputFilePath() const;
    std::vector<double> GetSnrThresholds() const;
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
    PlottingDetails GetPlottingDetails() const;
    bool ValidateSnrThresholds() const;
    bool ShouldSaveLog() const;

protected:
    // --- Event Handlers that remain in the Frame ---
    void OnExecuteClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnSplitterSashChanged(wxSplitterEvent& event);
    void OnNotebookPageChanged(wxNotebookEvent& event);
    void OnWorkerCompleted(wxCommandEvent& event);
    void OnWorkerUpdate(wxThreadEvent& event);
    void OnResultsCanvasPaint(wxPaintEvent& event);
    void OnChartPreviewPaint(wxPaintEvent& event);
    void OnGaugeTimer(wxTimerEvent& event);
    void OnRemoveAllFilesClick(wxCommandEvent& event);

    // --- UI Components ---
    wxPanel* m_resultsCanvasPanel;
    wxPanel* m_chartPreviewPanel;

    // --- Data for Drawing ---
    wxBitmap m_chartPreviewBitmap;

private:
    // --- Member variables ---
    std::unique_ptr<GuiPresenter> m_presenter;
    wxTimer* m_gaugeTimer;
    FileDropTarget* m_dropTarget;
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

class FileDropTarget : public wxFileDropTarget {
public:
    FileDropTarget(DynaRangeFrame* owner)
        : m_owner(owner)
    {
    }
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;

private:
    DynaRangeFrame* m_owner;
};