// File: gui/controllers/InputController.hpp
/**
 * @file gui/controllers/InputController.hpp
 * @brief Declares a controller class for the InputPanel's logic.
 */
#pragma once
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../../core/graphics/Constants.hpp"
#include <wx/event.h>
#include <string>
#include <vector>

// Forward declarations
class DynaRangeFrame;
class wxCommandEvent;
class wxScrollEvent;
class wxKeyEvent;
class wxArrayString;
class wxFileDirPickerEvent;

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
    std::vector<std::string> GetInputFiles() const;
    std::vector<double> GetChartCoords() const;
    std::string GetPrintPatchesFilename() const;
    int GetChartPatchesM() const; // Rows
    int GetChartPatchesN() const; // Cols
    RawChannelSelection GetRawChannelSelection() const;
    PlottingDetails GetPlottingDetails() const;
    bool ShouldSaveLog() const;
    bool ValidateSnrThresholds() const;
    /**
     * @brief (New Function) Checks if the user wants to generate individual plot files.
     * @return True if the "All ISOs" checkbox is checked, false otherwise.
     */
    bool ShouldGenerateIndividualPlots() const;
    // Methods to update the view
    void UpdateInputFileList(const std::vector<std::string>& files);
    void UpdateCommandPreview(const std::string& command);
    void AddDroppedFiles(const wxArrayString& filenames);

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

private:
    void PerformFileRemoval();
    bool IsSupportedRawFile(const wxString& filePath);
    
    DynaRangeFrame* m_frame; // Pointer to the parent frame to access its controls
    wxString m_lastDirectoryPath; ///< Stores the path of the last directory accessed by any file picker.
};