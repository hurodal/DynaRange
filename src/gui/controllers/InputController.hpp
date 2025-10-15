// File: gui/controllers/InputController.hpp
/**
 * @file gui/controllers/InputController.hpp
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
    /**
     * @brief (New Function) Checks if the user wants to generate individual plot files.
     * @return True if the "All ISOs" checkbox is checked, false otherwise.
     */
    bool ShouldGenerateIndividualPlots() const;

    // --- NUEVOS MÉTODOS PÚBLICOS ---
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

private:
    void PerformFileRemoval();
    bool IsSupportedRawFile(const wxString& filePath);
    wxPoint2DDouble PanelToImageCoords(const wxPoint& panelPoint) const;
    void UpdateCoordTextCtrls();
    
    DynaRangeFrame* m_frame;
    // Pointer to the parent frame to access its controls
    wxString m_lastDirectoryPath;
    ///< Stores the path of the last directory accessed by any file picker.
    
    wxImage m_rawPreviewImage;
    int m_originalRawWidth = 0;
    int m_originalRawHeight = 0;

    std::unique_ptr<ChartCornerInteractor> m_interactor;
    std::unique_ptr<PreviewOverlayRenderer> m_renderer;
};