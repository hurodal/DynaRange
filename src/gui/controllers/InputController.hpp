// File: src/gui/controllers/InputController.hpp
/**
 * @file src/gui/controllers/InputController.hpp
 * @brief Declares a controller class for the InputPanel's logic.
 */
#pragma once
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../../core/graphics/Constants.hpp"
#include "wx/image.h"
#include <wx/event.h>
#include <string>
#include <vector>
#include <memory>

#include "../preview_interaction/ChartCornerInteractor.hpp"
#include "../preview_interaction/PreviewOverlayRenderer.hpp"

// Forward declarations
class DynaRangeFrame;
class wxCommandEvent;
class wxScrollEvent;
class wxKeyEvent;
class wxArrayString;
class wxFileDirPickerEvent;
class wxPaintEvent;
class wxMouseEvent;
class wxMouseCaptureLostEvent;

class InputController {
public:
    explicit InputController(DynaRangeFrame* frame);
    // Getters that read directly from the controls
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
    std::string GetPrintPatchesFilename() const;
    int GetChartPatchesM() const; // Rows
    int GetChartPatchesN() const; // Cols
    RawChannelSelection GetRawChannelSelection() const;
    PlottingDetails GetPlottingDetails() const;
    std::vector<double> GetChartCoords() const;
    bool ShouldSaveLog() const;
    bool ValidateSnrThresholds() const;
    bool ShouldGenerateIndividualPlots() const;
    bool ShouldEstimateBlackLevel() const;
    bool ShouldEstimateSaturationLevel() const;

    // Methods to update the view
    void UpdateInputFileList(const std::vector<std::string>& files, int selected_index = -1);
    void UpdateCommandPreview(const std::string& command);
    void AddDroppedFiles(const wxArrayString& filenames);
    void DisplayPreviewImage(const std::string& path);

    // --- Event Handling Logic ---
    void OnAddFilesClick(wxCommandEvent& event);
    void OnPatchRatioSliderChanged(wxScrollEvent& event);
    void OnRemoveFilesClick(wxCommandEvent& event);
    void OnListBoxSelectionChanged(wxCommandEvent& event);
    void OnListBoxKeyDown(wxKeyEvent& event);
    void OnDrNormSliderChanged(wxScrollEvent& event);
    void OnDebugPatchesCheckBoxChanged(wxCommandEvent& event);
    void OnCalibrationFileChanged(wxFileDirPickerEvent& event);
    void OnInputChanged(wxEvent& event);
    void OnClearDarkFile(wxCommandEvent& event);
    void OnClearSaturationFile(wxCommandEvent& event);
    void OnInputChartPatchChanged(wxCommandEvent& event);
    void OnClearAllCoordsClick(wxCommandEvent& event);
    void OnPaintPreview(wxPaintEvent& event);
    void OnSizePreview(wxSizeEvent& event);
    void OnPreviewMouseDown(wxMouseEvent& event);
    void OnPreviewMouseUp(wxMouseEvent& event);
    void OnPreviewMouseMove(wxMouseEvent& event);
    void OnPreviewMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void OnGammaSliderChanged(wxScrollEvent& event);
    void OnPreviewKeyDown(wxKeyEvent& event);

private:
    void PerformFileRemoval();
    bool IsSupportedRawFile(const wxString& filePath);
    wxPoint2DDouble PanelToImageCoords(const wxPoint& panelPoint) const;
    void UpdateCoordTextCtrls();
    void ApplyGammaCorrection();
    void UpdatePreviewTransform();

    std::vector<wxPoint2DDouble> TransformGuiToRawCoords(const std::vector<wxPoint2DDouble>& guiCoords) const;

    DynaRangeFrame* m_frame;
    wxString m_lastDirectoryPath;
    
    // The original, unmodified preview image loaded from the RAW file.
    wxImage m_originalPreviewImage;
    // The gamma-corrected image that is actually shown on screen.
    wxImage m_displayPreviewImage;

    int m_originalActiveWidth = 0;
    int m_originalActiveHeight = 0;
    
    int m_rawOrientation = 0;

    std::unique_ptr<ChartCornerInteractor> m_interactor;
    std::unique_ptr<PreviewOverlayRenderer> m_renderer;

    // Stores the calculated scale factor for the preview image.
    double m_previewScale = 1.0;
    // Stores the top-left offset for the centered preview image.
    wxPoint2DDouble m_previewOffset = {0.0, 0.0};
};