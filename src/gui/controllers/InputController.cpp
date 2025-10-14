// File: gui/InputController.cpp
/**
 * @file gui/InputController.cpp
 * @brief Implements the InputController class.
 */
#include "InputController.hpp"
#include "../Constants.hpp"
#include "../DynaRangeFrame.hpp" // To access frame members and their members
#include "../GuiPresenter.hpp"   // To call presenter methods
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../graphics/Constants.hpp"
#include "../helpers/RawExtensionHelper.hpp"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <libraw/libraw.h>
#include <libraw/libraw_version.h>
#include <vector>
#include <set>
#include <string>
#include <sstream>

InputController::InputController(DynaRangeFrame* frame) : m_frame(frame) {
    // Dynamically populate the polynomial order choice control.
    m_frame->m_PlotChoice->Clear();
    for (const auto& order : VALID_POLY_ORDERS) {
        m_frame->m_PlotChoice->Append(std::to_string(order));
    }

    // Set the default selection based on the default value
    int default_index = -1;
    for (size_t i = 0; i < std::size(VALID_POLY_ORDERS); ++i) {
        if (VALID_POLY_ORDERS[i] == DEFAULT_POLY_ORDER) {
            default_index = i;
            break;
        }
    }
    if (default_index != -1) {
        m_frame->m_PlotChoice->SetSelection(default_index);
    }
    
    m_frame->m_darkValueTextCtrl->SetValue(wxString::Format("%.1f", DEFAULT_BLACK_LEVEL));
    m_frame->m_saturationValueTextCtrl->SetValue(wxString::Format("%.1f", DEFAULT_SATURATION_LEVEL));

    m_frame->m_outputTextCtrl->SetValue(DEFAULT_OUTPUT_FILENAME);
    m_frame->m_patchRatioSlider->SetValue(static_cast<int>(DEFAULT_PATCH_RATIO * 100));
    m_frame->m_patchRatioValueText->SetLabel(wxString::Format("%.2f", DEFAULT_PATCH_RATIO));
    m_frame->m_drNormalizationSlider->SetValue(static_cast<int>(DEFAULT_DR_NORMALIZATION_MPX));
    m_frame->m_drNormalizationValueText->SetLabel(wxString::Format("%.0fMpx", DEFAULT_DR_NORMALIZATION_MPX));
    m_frame->m_plotingChoice->SetSelection(DEFAULT_PLOT_MODE);
    m_frame->m_debugPatchesCheckBox->SetValue(false);
    m_frame->m_debugPatchesFileNameValue->Enable(false);
    m_frame->R_checkBox->SetValue(false);
    m_frame->G1_checkBox->SetValue(false);
    m_frame->G2_checkBox->SetValue(false);
    m_frame->B_checkBox->SetValue(false);
    m_frame->AVG_ChoiceValue->SetSelection(1); // Default is "Full" (index 1)
}

// --- Getters ---
std::string InputController::GetDarkFilePath() const { return std::string(m_frame->m_darkFilePicker->GetPath().mb_str()); }
std::string InputController::GetSaturationFilePath() const { return std::string(m_frame->m_saturationFilePicker->GetPath().mb_str()); }
double InputController::GetDarkValue() const { double val; m_frame->m_darkValueTextCtrl->GetValue().ToDouble(&val); return val; }
double InputController::GetSaturationValue() const { double val; m_frame->m_saturationValueTextCtrl->GetValue().ToDouble(&val); return val; }
double InputController::GetPatchRatio() const { return static_cast<double>(m_frame->m_patchRatioSlider->GetValue()) / 100.0; }
std::string InputController::GetOutputFilePath() const { return std::string(m_frame->m_outputTextCtrl->GetValue().mb_str()); }
double InputController::GetDrNormalization() const { return static_cast<double>(m_frame->m_drNormalizationSlider->GetValue()); }
int InputController::GetPolyOrder() const { return PolyOrderFromIndex(m_frame->m_PlotChoice->GetSelection()); }
int InputController::GetPlotMode() const { return m_frame->m_plotingChoice->GetSelection(); }
std::vector<std::string> InputController::GetInputFiles() const {
    std::vector<std::string> files;
    for (unsigned int i = 0; i < m_frame->m_rawFileslistBox->GetCount(); ++i) {
        files.push_back(std::string(m_frame->m_rawFileslistBox->GetString(i).mb_str()));
    }
    return files;
}
std::vector<double> InputController::GetSnrThresholds() const {
    std::vector<double> thresholds;
    std::string text = std::string(m_frame->m_snrThresholdsValues->GetValue().mb_str());
    std::stringstream ss(text);
    double value;

    while (ss >> value) {
        thresholds.push_back(value);
    }
    return thresholds;
}
// --- View Updaters ---
void InputController::UpdateInputFileList(const std::vector<std::string>& files) {
    m_frame->m_rawFileslistBox->Clear();
    for (const auto& file : files) {
        m_frame->m_rawFileslistBox->Append(file);
    }
}
void InputController::UpdateCommandPreview(const std::string& command) { m_frame->m_equivalentCliTextCtrl->ChangeValue(command); }

void InputController::PerformFileRemoval() {
    wxArrayInt selections;
    if (m_frame->m_rawFileslistBox->GetSelections(selections) == 0) return;
    
    std::vector<int> indices;
    for (size_t i = 0; i < selections.GetCount(); ++i) {
        indices.push_back(selections[i]);
    }
    // The frame holds the presenter, so we call it through the frame.
    m_frame->m_presenter->RemoveInputFiles(indices);
    m_frame->m_removeRawFilesButton->Enable(false);
}
bool InputController::IsSupportedRawFile(const wxString& filePath) {
    LibRaw p;
    return p.open_file(filePath.mb_str()) == LIBRAW_SUCCESS;
}
void InputController::AddDroppedFiles(const wxArrayString& filenames) {
    std::vector<std::string> files_to_add;
    wxArrayString rejected_files;
    for (const auto& file : filenames) {
        if (IsSupportedRawFile(file)) {
            files_to_add.push_back(std::string(file.mb_str()));
        } else {
            rejected_files.Add(wxFileName(file).GetFullName());
        }
    }
    if (!files_to_add.empty()) {
        m_frame->m_presenter->AddInputFiles(files_to_add);
    }

    if (!rejected_files.IsEmpty()) {
        wxString message = _("The following files were ignored because they are not recognized as supported RAW formats:\n\n");
        for (const auto& rejected : rejected_files) { message += "- " + rejected + "\n"; }
        wxMessageBox(message, _("Unsupported Files Skipped"), wxOK | wxICON_INFORMATION, m_frame);
    }
}

int InputController::GetChartPatchesM() const {
    long value = 0;
    m_frame->m_chartPatchRowValue1->GetValue().ToLong(&value);
    return static_cast<int>(value);
}

int InputController::GetChartPatchesN() const {
    long value = 0;
    m_frame->m_chartPatchColValue1->GetValue().ToLong(&value);
    return static_cast<int>(value);
}

std::string InputController::GetPrintPatchesFilename() const {
    if (m_frame->m_debugPatchesCheckBox->IsChecked()) {
        return std::string(m_frame->m_debugPatchesFileNameValue->GetValue().mb_str());
    }
    return "";
}

RawChannelSelection InputController::GetRawChannelSelection() const {
    RawChannelSelection selection;
    selection.R   = m_frame->R_checkBox->IsChecked();
    selection.G1  = m_frame->G1_checkBox->IsChecked();
    selection.G2  = m_frame->G2_checkBox->IsChecked();
    selection.B   = m_frame->B_checkBox->IsChecked();
    
    int avg_selection = m_frame->AVG_ChoiceValue->GetSelection();
    if (avg_selection >= 0 && avg_selection <= 2) {
        selection.avg_mode = static_cast<AvgMode>(avg_selection);
    } else {
        selection.avg_mode = AvgMode::Full; // Fallback
    }

    return selection;
}

DynaRange::Graphics::Constants::PlotOutputFormat InputController::GetPlotFormat() const {
    int selection = m_frame->m_plotFormatChoice->GetSelection();
    switch (selection) {
        case 1: return DynaRange::Graphics::Constants::PlotOutputFormat::PDF;
        case 2: return DynaRange::Graphics::Constants::PlotOutputFormat::SVG;
        case 0: // Fall-through for PNG
        default: return DynaRange::Graphics::Constants::PlotOutputFormat::PNG;
    }
}

bool InputController::ValidateSnrThresholds() const {
    std::string text = std::string(m_frame->m_snrThresholdsValues->GetValue().mb_str());
    if (text.empty()) {
        return true; // An empty list is valid (will use defaults)
    }

    std::stringstream ss(text);
    double value;
    
    // Try to read numbers separated by whitespace
    while (ss >> value) {
        // Successfully read a number
    }

    // After reading all numbers, the stream should be at the end.
    // If it's not, it means there were leftover non-numeric characters.
    return ss.eof();
}

bool InputController::ShouldSaveLog() const {
    return m_frame->m_saveLog->IsChecked();
}

PlottingDetails InputController::GetPlottingDetails() const {
    PlottingDetails details;
    details.show_scatters = m_frame->m_plotParamScattersCheckBox->IsChecked();
    details.show_curve = m_frame->m_plotParamCurveCheckBox->IsChecked();
    details.show_labels = m_frame->m_plotParamLabelsCheckBox->IsChecked();
    return details;
}

bool InputController::ShouldGenerateIndividualPlots() const {
    return m_frame->allIsosCheckBox->IsChecked();
}

void InputController::OnAddFilesClick(wxCommandEvent& event) {
    // 1. Get the list of extensions using the helper.
    const auto& supported_extensions = GuiHelpers::GetSupportedRawExtensions();

    // 2. Build the wildcard string using the centralized function.
    wxString filter = DynaRange::Gui::Constants::GetSupportedExtensionsWildcard(supported_extensions);
    
    // 3. Create and show the file dialog.
    wxFileDialog openFileDialog(m_frame, _("Select RAW files"), m_lastDirectoryPath, "", filter, wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL) { return; }
    
    wxArrayString paths;
    openFileDialog.GetPaths(paths);
    
    // 4. Remember the new path and update other pickers.
    if (paths.GetCount() > 0) {
        m_lastDirectoryPath = wxFileName(paths[0]).GetPath();
        m_frame->m_darkFilePicker->SetInitialDirectory(m_lastDirectoryPath);
        m_frame->m_saturationFilePicker->SetInitialDirectory(m_lastDirectoryPath);
    }
    
    AddDroppedFiles(paths);
}

void InputController::OnInputChanged(wxEvent& event) {
    m_frame->m_presenter->UpdateCommandPreview();
}

void InputController::OnPatchRatioSliderChanged(wxScrollEvent& event) {
    m_frame->m_patchRatioValueText->SetLabel(wxString::Format("%.2f", GetPatchRatio()));
    OnInputChanged(event);
}

void InputController::OnDrNormSliderChanged(wxScrollEvent& event) {
    m_frame->m_drNormalizationValueText->SetLabel(wxString::Format("%.0fMpx", GetDrNormalization()));
    OnInputChanged(event);
}

void InputController::OnRemoveFilesClick(wxCommandEvent& event) {
    PerformFileRemoval();
}

void InputController::OnListBoxSelectionChanged(wxCommandEvent& event) {
    wxArrayInt selections;
    m_frame->m_removeRawFilesButton->Enable(m_frame->m_rawFileslistBox->GetSelections(selections) > 0);
    event.Skip();
}

void InputController::OnListBoxKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_BACK) {
        PerformFileRemoval();
    }
    event.Skip();
}

void InputController::OnDebugPatchesCheckBoxChanged(wxCommandEvent& event) {
    bool is_checked = m_frame->m_debugPatchesCheckBox->IsChecked();
    m_frame->m_debugPatchesFileNameValue->Enable(is_checked);
    if (is_checked && m_frame->m_debugPatchesFileNameValue->GetValue().IsEmpty()) {
        m_frame->m_debugPatchesFileNameValue->SetValue("printpatches.png");
    }
    OnInputChanged(event);
}

void InputController::OnCalibrationFileChanged(wxFileDirPickerEvent& event)
{
    wxString path = event.GetPath();
    
    if (!path.IsEmpty()) {
        m_lastDirectoryPath = wxFileName(path).GetPath();
        m_frame->m_darkFilePicker->SetInitialDirectory(m_lastDirectoryPath);
        m_frame->m_saturationFilePicker->SetInitialDirectory(m_lastDirectoryPath);
    }

    // Delegate the logic of updating the file list to the presenter
    m_frame->m_presenter->UpdateCalibrationFiles();
    
    // The event must be skipped to allow the native control to process it.
    event.Skip();
}

void InputController::OnClearDarkFile(wxCommandEvent& event) {
    m_frame->m_darkFilePicker->SetPath("");
    // Delegate the logic of updating the file list to the presenter
    m_frame->m_presenter->UpdateCalibrationFiles();
}

void InputController::OnClearSaturationFile(wxCommandEvent& event) {
    m_frame->m_saturationFilePicker->SetPath("");
    // Delegate the logic of updating the file list to the presenter
    m_frame->m_presenter->UpdateCalibrationFiles();
}

void InputController::OnInputChartPatchChanged(wxCommandEvent& event) {
    if (m_frame->m_isUpdatingPatches) return;
    m_frame->m_isUpdatingPatches = true;

    m_frame->m_chartPatchRowValue->ChangeValue(m_frame->m_chartPatchRowValue1->GetValue());
    m_frame->m_chartPatchColValue->ChangeValue(m_frame->m_chartPatchColValue1->GetValue());
    
    OnInputChanged(event);

    m_frame->m_isUpdatingPatches = false;
    event.Skip();
}