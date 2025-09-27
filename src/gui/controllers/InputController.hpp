// File: gui/controllers/InputController.hpp
/**
 * @file gui/controllers/InputController.hpp
 * @brief Declares a controller class for the InputPanel's logic.
 */
#pragma once
#include <wx/event.h>
#include <string>
#include <vector>

// Forward declarations
class DynaRangeFrame;
class wxCommandEvent;
class wxScrollEvent;
class wxKeyEvent;
class wxArrayString;

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
    double GetSnrThreshold() const;
    double GetDrNormalization() const;
    int GetPolyOrder() const;
    int GetPlotMode() const;
    std::vector<std::string> GetInputFiles() const;
    std::vector<double> GetChartCoords() const;

    // Methods to update the view
    void UpdateInputFileList(const std::vector<std::string>& files);
    void UpdateCommandPreview(const std::string& command);
    void EnableExecuteButton(bool enable);
    void AddDroppedFiles(const wxArrayString& filenames);

    // Event handling logic, called by DynaRangeFrame's Bind()
    void OnAddFilesClick(wxCommandEvent& event);
    void OnPatchRatioSliderChanged(wxScrollEvent& event);
    void OnRemoveFilesClick(wxCommandEvent& event);
    void OnListBoxSelectionChanged(wxCommandEvent& event);
    void OnListBoxKeyDown(wxKeyEvent& event);
    void OnSnrSliderChanged(wxScrollEvent& event);
    void OnDrNormSliderChanged(wxScrollEvent& event);

private:
    void PerformFileRemoval();
    bool IsSupportedRawFile(const wxString& filePath);
    
    DynaRangeFrame* m_frame; // Pointer to the parent frame to access its controls
};