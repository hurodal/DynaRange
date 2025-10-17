// File: src/gui/controllers/InputController.hpp
/**
 * @file src/gui/controllers/InputController.hpp
 * @brief Declares a controller class for the InputPanel's logic.
 */
#pragma once
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../../core/graphics/Constants.hpp"
#include <wx/event.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class DynaRangeFrame;
class PreviewController;
class wxCommandEvent;
class wxScrollEvent;
class wxKeyEvent;
class wxArrayString;
class wxFileDirPickerEvent;

class InputController {
public:
    explicit InputController(DynaRangeFrame* frame);
    ~InputController(); // Explicit destructor declaration

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

private:
    void PerformFileRemoval();
    bool IsSupportedRawFile(const wxString& filePath);

    DynaRangeFrame* m_frame;
    wxString m_lastDirectoryPath;

    std::unique_ptr<PreviewController> m_previewController;
};