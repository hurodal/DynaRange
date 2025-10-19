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
#include "../../core/utils/OutputNamingContext.hpp" 
#include "../../core/utils/OutputFilenameGenerator.hpp"
#include "../GuiPresenter.hpp"
//#include "../graphics/Constants.hpp"
#include "../helpers/RawExtensionHelper.hpp"
#include <libraw/libraw.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

InputController::InputController(DynaRangeFrame* frame) : m_frame(frame), m_cachedExifName("") {
    m_previewController = std::make_unique<PreviewController>(frame);

    // Populate polynomial order choice
    m_frame->m_PlotChoice->Clear();
    for (const auto& order : VALID_POLY_ORDERS) {
        m_frame->m_PlotChoice->Append(std::to_string(order));
    }
    int default_poly_index = -1;
    for (size_t i = 0; i < std::size(VALID_POLY_ORDERS); ++i) {
        if (VALID_POLY_ORDERS[i] == DEFAULT_POLY_ORDER) {
            default_poly_index = i;
            break;
        }
    }
    if (default_poly_index != -1) {
        m_frame->m_PlotChoice->SetSelection(default_poly_index);
    }

    // Set other default values
    m_frame->m_darkValueTextCtrl->SetValue(wxString::Format("%.1f", DEFAULT_BLACK_LEVEL));
    m_frame->m_saturationValueTextCtrl->SetValue(wxString::Format("%.1f", DEFAULT_SATURATION_LEVEL));
    m_frame->m_patchRatioSlider->SetValue(static_cast<int>(DEFAULT_PATCH_RATIO * 100));
    m_frame->m_patchRatioValueText->SetLabel(wxString::Format("%.2f", DEFAULT_PATCH_RATIO));
    m_frame->m_drNormalizationSlider->SetValue(static_cast<int>(DEFAULT_DR_NORMALIZATION_MPX));
    m_frame->m_drNormalizationValueText->SetLabel(wxString::Format("%.0fMpx", DEFAULT_DR_NORMALIZATION_MPX));
    m_frame->m_debugPatchesCheckBox->SetValue(false);
    m_frame->m_debugPatchesFileNameValue->Enable(false);
    m_frame->AVG_ChoiceValue->SetSelection(DynaRange::Gui::Constants::AvgChoices::DEFAULT_INDEX);
    m_frame->m_gammaThumbSlider->Enable(false);
    m_frame->m_gammaThumbSlider->SetValue(DynaRange::Gui::Constants::DEFAULT_GAMMA_SLIDER_VALUE);
    m_frame->m_plotParamScattersCheckBox->SetValue(true);
    m_frame->m_plotParamCurveCheckBox->SetValue(true);
    m_frame->m_plotParamLabelsCheckBox->SetValue(true);
    m_frame->m_plotingChoice->SetSelection(3);

    // *** Establecer estado inicial de controles de nombre  ***
    bool initialAddSuffix = m_frame->m_subnameOutputcheckBox->IsChecked(); // Leer estado inicial
    bool initialUseExif = m_frame->m_fromExifOutputCheckBox->IsChecked(); // Leer estado inicial

    if (initialAddSuffix) {
        m_frame->m_fromExifOutputCheckBox->Enable(true);
        m_frame->m_subnameTextCtrl->Enable(!initialUseExif);
        if (initialUseExif) {
            m_frame->m_subnameTextCtrl->Clear();
        }
    } else {
        m_frame->m_fromExifOutputCheckBox->SetValue(false); // Forzar desmarcado
        m_frame->m_fromExifOutputCheckBox->Enable(false);
        m_frame->m_subnameTextCtrl->Clear();
        m_frame->m_subnameTextCtrl->Enable(false);
    }
    // *** FIN Estado inicial ***

    // Llamar al helper para establecer nombres iniciales (ahora usa DetermineEffectiveCameraName)
    UpdateDefaultFilenamesInUI();

    // Update AVG choice options
    UpdateAvgChoiceOptions();
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
std::string InputController::GetManualCameraName() const { return m_frame->m_subnameTextCtrl->GetValue().ToStdString(); }
bool InputController::GetUseExifNameFlag() const { return m_frame->m_fromExifOutputCheckBox->IsChecked(); }
bool InputController::GetUseSuffixFlag() const { return m_frame->m_subnameOutputcheckBox->IsChecked(); }

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
        // If the value is empty or is the default placeholder, return the sentinel value
        // that the core engine expects to trigger dynamic name generation.
        if (value.IsEmpty() || value == DEFAULT_PRINT_PATCHES_FILENAME) {
            return "_USE_DEFAULT_PRINT_PATCHES_";
        }
        // If the user has entered a custom name, respect it.
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
    // Update the available options in the Average mode choice
    UpdateAvgChoiceOptions();
    // Update the equivalent CLI command preview
    m_frame->m_presenter->UpdateCommandPreview();
    // Allow the event to propagate if needed (e.g., for text control updates)
    event.Skip();
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

    // Update default filenames (handles clearing/setting based on state)
    UpdateDefaultFilenamesInUI();

    // Trigger command preview update etc.
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

/**
 * @brief Private Helper: Determines the effective camera name based on GUI state.
 * @details Reads the state of m_subnameOutputcheckBox, m_fromExifOutputCheckBox,
 * and m_subnameTextCtrl to decide which camera name (EXIF, manual, or none)
 * should be used for output naming.
 * @return The effective camera name string, or an empty string if no suffix should be added.
 */
std::string InputController::DetermineEffectiveCameraName() const {
    // 1. Check if the suffix should be added at all
    if (!m_frame->m_subnameOutputcheckBox->IsChecked()) {
        return ""; // No suffix needed
    }

    // 2. Determine the source: EXIF or Manual
    bool useExif = m_frame->m_fromExifOutputCheckBox->IsChecked();
    std::string name_to_use;

    if (useExif) {
        // Use cached EXIF name
        // Ideally, this cache is updated whenever new files are added or analysis runs
        name_to_use = m_cachedExifName;
        // Fallback if cache is empty (e.g., before first run/file load)
        if (name_to_use.empty() && m_frame->m_presenter && !m_frame->m_presenter->GetLastReport().curve_data.empty()) {
             name_to_use = m_frame->m_presenter->GetLastReport().curve_data[0].camera_model;
        }

    } else {
        // Use manual name from text control
        name_to_use = m_frame->m_subnameTextCtrl->GetValue().ToStdString();
    }

    // Return the determined name (could still be empty if source was empty)
    return name_to_use;
}

/**
 * @brief Updates the default/placeholder filenames in the UI text controls (CSV, Debug Patches).
 * @details Generates default names using OutputFilenameGenerator based on the effective
 * camera name determined by the current GUI state. Updates controls cautiously.
 */
void InputController::UpdateDefaultFilenamesInUI() {
    // 1. Determinar el nombre efectivo de cámara AHORA
    std::string effectiveName = DetermineEffectiveCameraName(); // Llama al método (ahora público)

    // 2. Crear contexto SÓLO con el nombre efectivo
    OutputNamingContext ctx;
    ctx.effective_camera_name_for_output = effectiveName; // Usar el nombre determinado
    // Poblar EXIF para referencia interna de GetSafeCameraSuffix si fuera necesario (aunque ya no lo usa)
    ctx.camera_name_exif = m_cachedExifName;

    // 3. Generar nuevos nombres por defecto usando el nombre efectivo
    std::string new_default_csv = OutputFilenameGenerator::GenerateCsvFilename(ctx).string();
    std::string new_default_patches = "";
    if (m_frame->m_debugPatchesCheckBox->IsChecked()) {
        ctx.user_print_patches_filename = ""; // Asegurar que generamos el default
        new_default_patches = OutputFilenameGenerator::GeneratePrintPatchesFilename(ctx).string();
    }

    // 4. Actualizar control CSV si corresponde
    std::string current_csv_text = m_frame->m_outputTextCtrl->GetValue().ToStdString();
    if (current_csv_text.empty() || current_csv_text == m_currentDefaultCsvName) {
        m_frame->m_outputTextCtrl->ChangeValue(new_default_csv);
    }
    m_currentDefaultCsvName = new_default_csv;

    // 5. Actualizar control Patches si corresponde
    std::string current_patches_text = m_frame->m_debugPatchesFileNameValue->GetValue().ToStdString();
    if (m_frame->m_debugPatchesCheckBox->IsChecked()) {
         if (current_patches_text.empty() || current_patches_text == m_currentDefaultPatchesName) {
             m_frame->m_debugPatchesFileNameValue->ChangeValue(new_default_patches);
         }
         m_currentDefaultPatchesName = new_default_patches;
    } else {
         // Si se desmarca, limpiar el campo si contenía el default anterior
         if (!current_patches_text.empty() && current_patches_text == m_currentDefaultPatchesName) {
              m_frame->m_debugPatchesFileNameValue->Clear();
         }
         m_currentDefaultPatchesName = ""; // Resetear default cacheado
    }
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

/**
 * @brief Handles text changes specifically for the manual subname control ('m_subnameTextCtrl').
 * @details Updates default filenames if relevant and refreshes command preview.
 * @param event The command event details.
 */
void InputController::OnSubnameTextChanged(wxCommandEvent& event) {
    // Actualizar nombres por defecto solo si este control está habilitado
    // (lo que implica que addSuffix=true y useExif=false)
    if (m_frame->m_subnameTextCtrl->IsEnabled())
    {
        UpdateDefaultFilenamesInUI();
    }

    // Siempre actualizar comando preview
    if (m_frame->m_presenter) {
        m_frame->m_presenter->UpdateCommandPreview();
    }
    event.Skip();
}

/**
 * @brief Handles changes in the 'm_subnameOutputcheckBox' (Add Suffix).
 * @details Enables/disables/clears related controls according to the new logic,
 * updates default filenames and command preview.
 * @param event The command event details.
 */
void InputController::OnSubnameCheckBoxChanged(wxCommandEvent& event) {
    bool addSuffix = m_frame->m_subnameOutputcheckBox->IsChecked();

    if (addSuffix) {
        // Habilitar 'From Exif'
        m_frame->m_fromExifOutputCheckBox->Enable(true);
        // Habilitar/Deshabilitar campo manual según 'From Exif'
        bool useExif = m_frame->m_fromExifOutputCheckBox->IsChecked();
        m_frame->m_subnameTextCtrl->Enable(!useExif);
        // Limpiar campo manual si 'From Exif' está marcado
        if (useExif) {
             m_frame->m_subnameTextCtrl->Clear();
        }
    } else {
        // Deshabilitar y desmarcar 'From Exif'
        m_frame->m_fromExifOutputCheckBox->SetValue(false);
        m_frame->m_fromExifOutputCheckBox->Enable(false);
        // Deshabilitar y limpiar campo manual
        m_frame->m_subnameTextCtrl->Clear();
        m_frame->m_subnameTextCtrl->Enable(false);
    }

    // Actualizar nombres por defecto y comando
    UpdateDefaultFilenamesInUI();
    if (m_frame->m_presenter) {
        m_frame->m_presenter->UpdateCommandPreview();
    }

    event.Skip();
}

/**
 * @brief Handles changes in the 'm_fromExifOutputCheckBox'.
 * @details Enables/disables manual name field, clears it if needed,
 * updates default filenames and command preview.
 * @param event The command event details.
 */
void InputController::OnFromExifCheckBoxChanged(wxCommandEvent& event) {
    // Solo reaccionar si el checkbox "Add Suffix" está habilitado
    if (m_frame->m_subnameOutputcheckBox->IsChecked()) {
        bool useExif = m_frame->m_fromExifOutputCheckBox->IsChecked();
        m_frame->m_subnameTextCtrl->Enable(!useExif);
        if (useExif) {
            m_frame->m_subnameTextCtrl->Clear();
        }

        // Actualizar nombres por defecto y comando
        UpdateDefaultFilenamesInUI();
        if (m_frame->m_presenter) {
            m_frame->m_presenter->UpdateCommandPreview();
        }
    }
    event.Skip();
}

void InputController::OnClearAllCoordsClick(wxCommandEvent& event)
{
    // Limpia explícitamente los 8 campos de texto de las coordenadas
    m_frame->m_coordX1Value->Clear();
    m_frame->m_coordY1Value->Clear();
    m_frame->m_coordX2Value->Clear();
    m_frame->m_coordY2Value->Clear();
    m_frame->m_coordX3Value->Clear();
    m_frame->m_coordY3Value->Clear();
    m_frame->m_coordX4Value->Clear();
    m_frame->m_coordY4Value->Clear();

    // Llama al método del PreviewController para resetear explícitamente la interacción visual.
    // Esto también llamará internamente a UpdateCoordTextCtrls para asegurarse
    // de que los valores (ahora vacíos) son consistentes con el estado reseteado.
    if(m_previewController) {
        m_previewController->ResetCornerInteraction();
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

void InputController::UpdateAvgChoiceOptions() {
    using namespace DynaRange::Gui::Constants::AvgChoices;

    // Get current selection before clearing
    int currentSelectionIndex = m_frame->AVG_ChoiceValue->GetSelection();
    wxString currentSelectionString = m_frame->AVG_ChoiceValue->GetStringSelection();

    // Check if any individual channel is selected
    bool anyChannelSelected = m_frame->R_checkBox->IsChecked() ||
                              m_frame->G1_checkBox->IsChecked() ||
                              m_frame->G2_checkBox->IsChecked() ||
                              m_frame->B_checkBox->IsChecked();

    // Rebuild the choice list
    m_frame->AVG_ChoiceValue->Clear();
    m_frame->AVG_ChoiceValue->Append(_(No)); // Use _() for translation at runtime
    m_frame->AVG_ChoiceValue->Append(_(Full));
    bool selectedOptionAvailable = false;
    if (anyChannelSelected) {
        m_frame->AVG_ChoiceValue->Append(_(Selected));
        selectedOptionAvailable = true; // "Only Selected" is now an option
    }

    // Determine the new selection
    int newSelectionIndex = DEFAULT_INDEX; // Default to "Full"

    // Try to restore previous selection if possible
    if (currentSelectionString == _(No)) {
        newSelectionIndex = IDX_NO;
    } else if (currentSelectionString == _(Full)) {
        newSelectionIndex = IDX_FULL;
    } else if (currentSelectionString == _(Selected)) {
        if (selectedOptionAvailable) {
            // Find the index of "Only Selected" (it might be 2)
            newSelectionIndex = m_frame->AVG_ChoiceValue->FindString(_(Selected));
        } else {
            // "Only Selected" was chosen but is no longer valid, revert to default
            newSelectionIndex = DEFAULT_INDEX;
        }
    }

    // Ensure the index is valid before setting
    if (newSelectionIndex < 0 || newSelectionIndex >= (int)m_frame->AVG_ChoiceValue->GetCount()) {
        newSelectionIndex = DEFAULT_INDEX; // Fallback safety
    }

    m_frame->AVG_ChoiceValue->SetSelection(newSelectionIndex);
}