// File: src/gui/controllers/InputController.cpp
/**
 * @file gui/controllers/InputController.cpp
 * @brief Implements the InputController class.
 */
#include "InputController.hpp"
#include "PreviewController.hpp" // Include the new controller's full definition
#include "../Constants.hpp"
#include "../DynaRangeFrame.hpp"
#include "../GuiPresenter.hpp"
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../graphics/Constants.hpp"
#include "../helpers/RawExtensionHelper.hpp"
#include <libraw/libraw.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

InputController::InputController(DynaRangeFrame* frame) : m_frame(frame) {
    // Instantiate the new controller responsible for the preview panel
    m_previewController = std::make_unique<PreviewController>(frame);

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

    // Ensure the gamma slider is disabled by default.
    m_frame->m_gammaThumbSlider->Enable(false);
}

// Explicit destructor implementation
InputController::~InputController() = default;

// --- Getters ---
std::string InputController::GetDarkFilePath() const { return std::string(m_frame->m_darkFilePicker->GetPath().mb_str()); }
std::string InputController::GetSaturationFilePath() const { return std::string(m_frame->m_saturationFilePicker->GetPath().mb_str());
}
double InputController::GetDarkValue() const { double val; m_frame->m_darkValueTextCtrl->GetValue().ToDouble(&val); return val; }
double InputController::GetSaturationValue() const { double val; m_frame->m_saturationValueTextCtrl->GetValue().ToDouble(&val); return val;
}
double InputController::GetPatchRatio() const { return static_cast<double>(m_frame->m_patchRatioSlider->GetValue()) / 100.0; }
std::string InputController::GetOutputFilePath() const { return std::string(m_frame->m_outputTextCtrl->GetValue().mb_str()); }
double InputController::GetDrNormalization() const { return static_cast<double>(m_frame->m_drNormalizationSlider->GetValue());
}
int InputController::GetPolyOrder() const { return PolyOrderFromIndex(m_frame->m_PlotChoice->GetSelection()); }
int InputController::GetPlotMode() const { return m_frame->m_plotingChoice->GetSelection();
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

void InputController::UpdateInputFileList(const std::vector<std::string>& files, int selected_index)
{
    m_frame->m_rawFileslistBox->Clear();
    for (size_t i = 0; i < files.size(); ++i) {
        std::string display_name = files[i];
        if (static_cast<int>(i) == selected_index) {
            display_name = "->" + display_name;
        }
        m_frame->m_rawFileslistBox->Append(display_name);
    }

    if (selected_index != -1) {
        m_frame->m_rawFileslistBox->EnsureVisible(selected_index);
    }
}

void InputController::UpdateCommandPreview(const std::string& command) { m_frame->m_equivalentCliTextCtrl->ChangeValue(command);
}

void InputController::PerformFileRemoval() {
    wxArrayInt selections;
    if (m_frame->m_rawFileslistBox->GetSelections(selections) == 0) return;
    
    std::vector<int> indices;
    for (size_t i = 0; i < selections.GetCount(); ++i) {
        indices.push_back(selections[i]);
    }
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
        for (const auto& rejected : rejected_files) { message += "- " + rejected + "\n";
        }
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
        wxString value = m_frame->m_debugPatchesFileNameValue->GetValue();
        if (value.IsEmpty()) {
            return "_USE_DEFAULT_PRINT_PATCHES_";
        }
        return std::string(value.mb_str());
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
        selection.avg_mode = AvgMode::Full;
    }

    return selection;
}

DynaRange::Graphics::Constants::PlotOutputFormat InputController::GetPlotFormat() const {
    int selection = m_frame->m_plotFormatChoice->GetSelection();
    switch (selection) {
        case 1: return DynaRange::Graphics::Constants::PlotOutputFormat::PDF;
        case 2: return DynaRange::Graphics::Constants::PlotOutputFormat::SVG;
        case 0:
        default: return DynaRange::Graphics::Constants::PlotOutputFormat::PNG;
    }
}

bool InputController::ValidateSnrThresholds() const {
    std::string text = std::string(m_frame->m_snrThresholdsValues->GetValue().mb_str());
    if (text.empty()) {
        return true;
    }

    std::stringstream ss(text);
    double value;
    while (ss >> value) {
        // Successfully read a number
    }
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
    const auto& supported_extensions = GuiHelpers::GetSupportedRawExtensions();
    wxString filter = DynaRange::Gui::Constants::GetSupportedExtensionsWildcard(supported_extensions);
    wxFileDialog openFileDialog(m_frame, _("Select RAW files"), m_lastDirectoryPath, "", filter, wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (openFileDialog.ShowModal() == wxID_CANCEL) { return; }
    
    wxArrayString paths;
    openFileDialog.GetPaths(paths);
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
    m_frame->m_presenter->UpdateCalibrationFiles();
    event.Skip();
}

void InputController::OnClearDarkFile(wxCommandEvent& event) {
    m_frame->m_darkFilePicker->SetPath("");
    m_frame->m_presenter->UpdateCalibrationFiles();
}

void InputController::OnClearSaturationFile(wxCommandEvent& event) {
    m_frame->m_saturationFilePicker->SetPath("");
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

std::vector<double> InputController::GetChartCoords() const {
    std::vector<double> coords;
    std::vector<wxTextCtrl*> controls = {
        m_frame->m_coordX1Value, m_frame->m_coordY1Value,
        m_frame->m_coordX2Value, m_frame->m_coordY2Value,
        m_frame->m_coordX3Value, m_frame->m_coordY3Value,
        m_frame->m_coordX4Value, m_frame->m_coordY4Value
    };
    for (wxTextCtrl* control : controls) {
        if (control->GetValue().IsEmpty()) {
            return {};
        }
        double val;
        if (!control->GetValue().ToDouble(&val)) {
            return {};
        }
        coords.push_back(val);
    }
    return coords;
}

void InputController::DisplayPreviewImage(const std::string& path)
{
    if (m_previewController) {
        m_previewController->DisplayPreviewImage(path);
    }
}

void InputController::OnClearAllCoordsClick(wxCommandEvent& event)
{
    // Limpia los 8 campos de texto de las coordenadas
    m_frame->m_coordX1Value->Clear();
    m_frame->m_coordY1Value->Clear();
    m_frame->m_coordX2Value->Clear();
    m_frame->m_coordY2Value->Clear();
    m_frame->m_coordX3Value->Clear();
    m_frame->m_coordY3Value->Clear();
    m_frame->m_coordX4Value->Clear();
    m_frame->m_coordY4Value->Clear();

    if(m_previewController) {
        // Refresh the preview with the current file to reset corner handles.
        m_previewController->DisplayPreviewImage(m_frame->m_currentPreviewFile);
    }

    // Actualiza el comando CLI
    m_frame->m_presenter->UpdateCommandPreview();
}

bool InputController::ShouldEstimateBlackLevel() const
{
    bool isFilePickerEmpty = m_frame->m_darkFilePicker->GetPath().IsEmpty();
    bool isTextBoxEmpty = m_frame->m_darkValueTextCtrl->GetValue().IsEmpty();
    return isFilePickerEmpty && isTextBoxEmpty;
}

bool InputController::ShouldEstimateSaturationLevel() const
{
    bool isFilePickerEmpty = m_frame->m_saturationFilePicker->GetPath().IsEmpty();
    bool isTextBoxEmpty = m_frame->m_saturationValueTextCtrl->GetValue().IsEmpty();
    return isFilePickerEmpty && isTextBoxEmpty;
}