// File: src/gui/controllers/InputController.hpp
/**
 * @file src/gui/controllers/InputController.hpp
 * @brief Declares a controller class for the InputPanel's logic.
 */
#pragma once
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../../core/graphics/Constants.hpp"
#include <wx/event.h> // Needed for event handler parameters
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

// Forward declarations
class DynaRangeFrame;
class PreviewController;
class wxCommandEvent;
class wxScrollEvent;
class wxKeyEvent;
class wxArrayString;
class wxFileDirPickerEvent;

/**
 * @class InputController
 * @brief Manages the logic and event handling for the main input tab/panel.
 * @details Interacts with the GUI controls on the input panel, validates input,
 * communicates with the GuiPresenter, and manages the PreviewController.
 */
class InputController {
public:
    /**
     * @brief Constructor. Initializes default values and sets up event handling.
     * @param frame Pointer to the main DynaRangeFrame.
     */
    explicit InputController(DynaRangeFrame* frame);
    /** @brief Destructor. */
    ~InputController();

    // --- Getters for retrieving current settings from UI controls ---
    /** @brief Gets the full path selected in the dark file picker. */
    std::string GetDarkFilePath() const;
    /** @brief Gets the full path selected in the saturation file picker. */
    std::string GetSaturationFilePath() const;
    /** @brief Gets the numeric black level value from the text control. */
    double GetDarkValue() const;
    /** @brief Gets the numeric saturation level value from the text control. */
    double GetSaturationValue() const;
    /** @brief Gets the patch ratio value (0.0-1.0) from the slider. */
    double GetPatchRatio() const;
    /** @brief Gets the output CSV filename/path from the text control. */
    std::string GetOutputFilePath() const;
    /** @brief Gets the list of SNR thresholds (as doubles) from the text control. */
    std::vector<double> GetSnrThresholds() const;
    /** @brief Gets the DR normalization value (in Mpx) from the slider. */
    double GetDrNormalization() const;
    /** @brief Gets the selected polynomial order (2 or 3). */
    int GetPolyOrder() const;
    /** @brief Gets the selected plot mode (0=No, 1=Graph, 2=Graph+ShortCmd, 3=Graph+LongCmd). */
    int GetPlotMode() const;
    /** @brief Gets the selected plot output format (PNG, PDF, SVG). */
    DynaRange::Graphics::Constants::PlotOutputFormat GetPlotFormat() const;
    /** @brief Gets the filename for the debug patches image (or sentinel value). */
    std::string GetPrintPatchesFilename() const;
    /** @brief Gets the number of chart patch rows (M) for analysis. */
    int GetChartPatchesM() const;
    /** @brief Gets the number of chart patch columns (N) for analysis. */
    int GetChartPatchesN() const;
    /** @brief Gets the current selection state for RAW channels and averaging mode. */
    RawChannelSelection GetRawChannelSelection() const;
    /** @brief Gets the current selection state for plot element visibility. */
    PlottingDetails GetPlottingDetails() const;
    /** @brief Gets the manually entered chart corner coordinates. Returns empty if invalid/incomplete. */
    std::vector<double> GetChartCoords() const;
    /** @brief Checks if the 'Save Log' checkbox is checked. */
    bool ShouldSaveLog() const;
    /** @brief Validates the format of the SNR thresholds input string. */
    bool ValidateSnrThresholds() const;
    /** @brief Checks if the 'All ISOs' (generate individual plots) checkbox is checked. */
    bool ShouldGenerateIndividualPlots() const;
    /** @brief Checks if the black level should be estimated (no file or value provided). */
    bool ShouldEstimateBlackLevel() const;
    /** @brief Checks if the saturation level should be estimated (no file or value provided). */
    bool ShouldEstimateSaturationLevel() const;
    /** @brief Gets the manual camera name entered by the user. */
    std::string GetManualCameraName() const;
    /** @brief Gets the state of the 'Use EXIF Name' checkbox. */
    bool GetUseExifNameFlag() const;
    /** @brief Gets the state of the 'Add Suffix' checkbox. */
    bool GetUseSuffixFlag() const;
    /** @brief Checks if the 'Full debug' checkbox is checked. */
    bool ShouldGenerateFullDebug() const;

    /**
     * @brief Determines the effective camera name based on GUI control states.
     * @details Reads the state of m_subnameOutputcheckBox, m_fromExifOutputCheckBox,
     * and m_subnameTextCtrl to decide which camera name (EXIF, manual, or none)
     * should be used for output naming. Needs access to cached EXIF name.
     * @return The effective camera name string, or an empty string if no suffix should be added.
     */
    std::string DetermineEffectiveCameraName() const;


    // --- Methods called by Presenter/Frame to update the view ---
    /** @brief Updates the listbox displaying input RAW files. */
    void UpdateInputFileList(const std::vector<std::string>& files, int selected_index = -1);
    /** @brief Updates the text control showing the equivalent CLI command. */
    void UpdateCommandPreview(const std::string& command);
    /** @brief Handles files dropped onto the frame, filtering and adding valid RAWs. */
    void AddDroppedFiles(const wxArrayString& filenames);
    /** @brief Instructs the PreviewController to display a specific image. */
    void DisplayPreviewImage(const std::string& path);

    // --- Event Handling Methods (called by wxWidgets event system) ---
    /** @brief Handles the 'Add RAW Files...' button click. */
    void OnAddFilesClick(wxCommandEvent& event);
    /** @brief Handles changes in the patch ratio slider. */
    void OnPatchRatioSliderChanged(wxScrollEvent& event);
    /** @brief Handles the 'Remove Selected' button click. */
    void OnRemoveFilesClick(wxCommandEvent& event);
    /** @brief Handles selection changes in the RAW file listbox. */
    void OnListBoxSelectionChanged(wxCommandEvent& event);
    /** @brief Handles key presses (Delete/Backspace) in the RAW file listbox. */
    void OnListBoxKeyDown(wxKeyEvent& event);
    /** @brief Handles changes in the DR normalization slider. */
    void OnDrNormSliderChanged(wxScrollEvent& event);
    /** @brief Handles changes in the 'Debug patches' checkbox. */
    void OnDebugPatchesCheckBoxChanged(wxCommandEvent& event);
    /** @brief Handles changes in the dark/saturation file pickers. */
    void OnCalibrationFileChanged(wxFileDirPickerEvent& event);
    /** @brief Generic handler for simple input changes triggering command preview update. */
    void OnInputChanged(wxEvent& event);
    /** @brief Handles the 'Clear' button click for the dark file picker. */
    void OnClearDarkFile(wxCommandEvent& event);
    /** @brief Handles the 'Clear' button click for the saturation file picker. */
    void OnClearSaturationFile(wxCommandEvent& event);
    /** @brief Handles text changes in the chart patch controls on the Input tab. */
    void OnInputChartPatchChanged(wxCommandEvent& event);
    /** @brief Handles the 'Clear All' button click for manual coordinates. */
    void OnClearAllCoordsClick(wxCommandEvent& event);
    /** @brief Handles changes specifically for the 'From Exif' checkbox. */
    void OnFromExifCheckBoxChanged(wxCommandEvent& event);
    /** @brief Handles changes specifically for the 'Subname' (add suffix) checkbox. */
    void OnSubnameCheckBoxChanged(wxCommandEvent& event);
    /** @brief Handles text changes specifically for the manual subname control. */
    void OnSubnameTextChanged(wxCommandEvent& event);


private:
    // --- Private Helper Methods ---
    /** @brief Updates the items and selection of the AVG mode choice control. */
    void UpdateAvgChoiceOptions();
    /** @brief Performs file removal based on the current listbox selection. */
    void PerformFileRemoval();
    /** @brief Checks if a given file path points to a likely supported RAW file. */
    bool IsSupportedRawFile(const wxString& filePath);
    /** @brief Updates the default filenames shown in the UI text controls. */
    void UpdateDefaultFilenamesInUI();
    // NOTE: DetermineEffectiveCameraName moved to public

    // --- Member Variables ---
    DynaRangeFrame* m_frame; ///< Pointer back to the main frame/view
    wxString m_lastDirectoryPath; ///< Caches the last used directory for file dialogs
    std::unique_ptr<PreviewController> m_previewController; ///< Controller for the preview panel
    std::string m_cachedExifName; ///< Stores the last known EXIF camera name
    std::string m_currentDefaultCsvName;      ///< Stores the last default CSV name set in the UI
    std::string m_currentDefaultPatchesName;  ///< Stores the last default Patches name set in the UI
};