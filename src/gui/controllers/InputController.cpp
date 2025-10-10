// File: gui/InputController.cpp
/**
 * @file gui/InputController.cpp
 * @brief Implements the InputController class.
 */
#include "InputController.hpp"
#include "../DynaRangeFrame.hpp" // To access frame members and their members
#include "../GuiPresenter.hpp"   // To call presenter methods
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../graphics/Constants.hpp"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <libraw/libraw.h>
#include <libraw/libraw_version.h>
#include <vector>
#include <set>
#include <string>
#include <sstream>

namespace { // Anonymous namespace for internal helpers

const std::vector<std::string>& GetSupportedRawExtensions() {
    static std::vector<std::string> extensions;
    if (extensions.empty()) {
        // Verificamos si la versión de LibRaw es >= 0.22.0 usando macros oficiales
        #if defined(LIBRAW_MAJOR_VERSION) && defined(LIBRAW_MINOR_VERSION)
            #if LIBRAW_MAJOR_VERSION > 0 || (LIBRAW_MAJOR_VERSION == 0 && LIBRAW_MINOR_VERSION >= 22)
                // LibRaw 0.22.0+ → usamos la API dinámica
                LibRaw proc;
                int count = 0;
                const char** ext_list = proc.get_supported_extensions_list(&count);
                if (ext_list) {
                    std::set<std::string> unique;
                    for (int i = 0; i < count; ++i) {
                        if (!ext_list[i]) continue;
                        std::string ext(ext_list[i]);
                        if (!ext.empty() && ext[0] == '.') {
                            ext = ext.substr(1);
                        }
                        std::transform(ext.begin(), ext.end(), ext.begin(),
                            [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
                        if (!ext.empty()) {
                            unique.insert(ext);
                        }
                    }
                    extensions.assign(unique.begin(), unique.end());
                }
            #endif
        #endif

        // Si no se pudo obtener dinámicamente (versión antigua o fallo), usar fallback
        if (extensions.empty()) {
            // Lista completa de extensiones soportadas por LibRaw (hasta 0.21.4)
            static const std::vector<std::string> fallback = {
                "3fr", "ari", "arw", "bay", "crw", "cr2", "cr3",
                "cap", "data", "dcs", "dcr", "dng", "drf", "eip",
                "erf", "fff", "gpr", "iiq", "k25", "kdc", "mdc",
                "mef", "mos", "mrw", "nef", "nrw", "obm", "orf",
                "pef", "ptx", "pxn", "r3d", "raf", "raw", "rwl",
                "rw2", "rwz", "sr2", "srf", "srw", "x3f"
            };
            extensions = fallback;
        }
    }
    return extensions;
}

} // end anonymous namespace

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
    m_frame->AVG_checkBox->SetValue(true); // Default is AVG only    
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

// --- Event Logic ---
void InputController::OnAddFilesClick(wxCommandEvent& event) {
    // 1. Obtener la lista de extensiones dinámicamente.
    const auto& supported_extensions = GetSupportedRawExtensions();

    // 2. Construir la cadena de comodines (wildcard).
    wxString wildcard;
    for (size_t i = 0; i < supported_extensions.size(); ++i) {
        wxString ext_lower = supported_extensions[i];
        wxString ext_upper = ext_lower.Upper();
        
        wildcard += wxString::Format("*.%s;*.%s", ext_lower, ext_upper);
        
        if (i < supported_extensions.size() - 1) {
            wildcard += ";";
        }
    }

    // 3. Construir la cadena de filtro final para el diálogo.
    wxString filter = wxString::Format(
        _("RAW files (%s)|%s|All files (*.*)|*.*"),
        wildcard, wildcard
    );

    // 4. Crear y mostrar el diálogo de selección de ficheros.
    wxFileDialog openFileDialog(m_frame, _("Select RAW files"), "", "", filter, wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL) { return; }
    
    wxArrayString paths;
    openFileDialog.GetPaths(paths);
    AddDroppedFiles(paths);
}

void InputController::OnPatchRatioSliderChanged(wxScrollEvent& event) {
    m_frame->m_patchRatioValueText->SetLabel(wxString::Format("%.2f", GetPatchRatio()));
    m_frame->OnInputChanged(event); // Notify frame to update preview
}

void InputController::OnDrNormSliderChanged(wxScrollEvent& event) {
    m_frame->m_drNormalizationValueText->SetLabel(wxString::Format("%.0fMpx", GetDrNormalization()));
    m_frame->OnInputChanged(event);
}
void InputController::OnRemoveFilesClick(wxCommandEvent& event) { PerformFileRemoval(); }
void InputController::OnListBoxSelectionChanged(wxCommandEvent& event) {
    wxArrayInt selections;
    m_frame->m_removeRawFilesButton->Enable(m_frame->m_rawFileslistBox->GetSelections(selections) > 0);
}
void InputController::OnListBoxKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_BACK) {
        PerformFileRemoval();
    }
    event.Skip();
}
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

std::vector<double> InputController::GetChartCoords() const {
    std::vector<double> coords;
    coords.reserve(8);

    // List of all coordinate text controls
    std::vector<wxTextCtrl*> controls = {
        m_frame->m_coordX1Value, m_frame->m_coordY1Value,
        m_frame->m_coordX2Value, m_frame->m_coordY2Value,
        m_frame->m_coordX3Value, m_frame->m_coordY3Value,
        m_frame->m_coordX4Value, m_frame->m_coordY4Value
    };

    for (wxTextCtrl* control : controls) {
        wxString value_str = control->GetValue();
        // If any field is empty, we consider the whole set invalid.
        if (value_str.IsEmpty()) {
            return {}; // Return empty vector
        }
        double val;
        if (!value_str.ToDouble(&val)) {
            // If conversion fails for any field, also return empty.
            return {};
        }
        coords.push_back(val);
    }

    return coords;
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

void InputController::OnDebugPatchesCheckBoxChanged(wxCommandEvent& event) {
    bool is_checked = m_frame->m_debugPatchesCheckBox->IsChecked();
    m_frame->m_debugPatchesFileNameValue->Enable(is_checked);

    if (is_checked && m_frame->m_debugPatchesFileNameValue->GetValue().IsEmpty()) {
        m_frame->m_debugPatchesFileNameValue->SetValue("printpatches.png");
    }
}

RawChannelSelection InputController::GetRawChannelSelection() const {
    RawChannelSelection selection;
    selection.R   = m_frame->R_checkBox->IsChecked();
    selection.G1  = m_frame->G1_checkBox->IsChecked();
    selection.G2  = m_frame->G2_checkBox->IsChecked();
    selection.B   = m_frame->B_checkBox->IsChecked();
    selection.AVG = m_frame->AVG_checkBox->IsChecked();
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