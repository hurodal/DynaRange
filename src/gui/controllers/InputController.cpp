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
#include "../helpers/CvWxImageConverter.hpp"
#include "wx/dcbuffer.h"
#include "wx/graphics.h"
#include "wx/log.h"
#include <libraw/libraw.h>
#include <libraw/libraw_version.h>
#include <opencv2/imgproc.hpp>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/graphics.h>
#include <wx/msgdlg.h>

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
    m_frame->AVG_ChoiceValue->SetSelection(1);
    // Default is "Full" (index 1)

    // Instanciar el interactor y el renderizador
    m_interactor = std::make_unique<ChartCornerInteractor>();
    m_renderer = std::make_unique<PreviewOverlayRenderer>();

    // Bind events for the preview panel
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_PAINT, &InputController::OnPaintPreview, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_SIZE, &InputController::OnSizePreview, this);
    
    // Bind new mouse events for interaction
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_DOWN, &InputController::OnPreviewMouseDown, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_UP, &InputController::OnPreviewMouseUp, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOTION, &InputController::OnPreviewMouseMove, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &InputController::OnPreviewMouseCaptureLost, this);
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

    // Si hay un elemento seleccionado, nos aseguramos de que sea visible.
    if (selected_index != -1) {
        m_frame->m_rawFileslistBox->EnsureVisible(selected_index);
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

void InputController::DisplayPreviewImage(const std::string& path)
{
    if (path.empty()) {
        m_rawPreviewImage = wxImage(); // Clear image
        m_originalRawWidth = 0;
        m_originalRawHeight = 0;
    } else {
        RawFile raw_file(path);
        if (raw_file.Load()) {
            cv::Mat full_res_mat = raw_file.GetProcessedImage();
            if (full_res_mat.empty()) {
                m_rawPreviewImage = wxImage();
                wxLogError("Could not get processed image from RAW file: %s", path);
            } else {
                m_originalRawWidth = full_res_mat.cols;
                m_originalRawHeight = full_res_mat.rows;
                constexpr int MAX_PREVIEW_DIMENSION = 1920;
                cv::Mat preview_mat;
                if (m_originalRawWidth > MAX_PREVIEW_DIMENSION || m_originalRawHeight > MAX_PREVIEW_DIMENSION) {
                    double scale = static_cast<double>(MAX_PREVIEW_DIMENSION) / std::max(m_originalRawWidth, m_originalRawHeight);
                    cv::resize(full_res_mat, preview_mat, cv::Size(), scale, scale, cv::INTER_AREA);
                } else {
                    preview_mat = full_res_mat;
                }
                m_rawPreviewImage = GuiHelpers::CvMatToWxImage(preview_mat);
            }
        } else {
            m_rawPreviewImage = wxImage(); // Clear on failure
            m_originalRawWidth = 0;
            m_originalRawHeight = 0;
            wxLogError("Could not load RAW file for preview: %s", path);
        }
    }
    
    // Informa al interactor del nuevo tamaño de la imagen (o 0,0 si se borró)
    if (m_rawPreviewImage.IsOk()) {
        m_interactor->SetImageSize(m_rawPreviewImage.GetSize());
    } else {
        m_interactor->SetImageSize(wxSize(0,0));
    }

    m_frame->m_rawImagePreviewPanel->Refresh();
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

    // Resetea el estado interno del interactor a las esquinas de la imagen.
    m_interactor->ResetCorners();

    // Fuerza un redibujado del panel de previsualización para mostrar los manejadores en su nueva posición.
    m_frame->m_rawImagePreviewPanel->Refresh();

    // Actualiza el comando CLI equivalente para reflejar que ya no hay coordenadas manuales.
    m_frame->m_presenter->UpdateCommandPreview();
}

void InputController::OnPaintPreview(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(m_frame->m_rawImagePreviewPanel);
    dc.Clear();
    if (m_rawPreviewImage.IsOk()) {
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            double img_w = m_rawPreviewImage.GetWidth();
            double img_h = m_rawPreviewImage.GetHeight();
            const wxSize panel_size = dc.GetSize();
            
            double scale_factor = std::min((double)panel_size.GetWidth() / img_w, (double)panel_size.GetHeight() / img_h);
            double final_width = img_w * scale_factor;
            double final_height = img_h * scale_factor;
            double offset_x = (panel_size.GetWidth() - final_width) / 2.0;
            double offset_y = (panel_size.GetHeight() - final_height) / 2.0;
            
            wxImage displayImage = m_rawPreviewImage.Copy();
            displayImage.Rescale(final_width, final_height, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmapToDraw(displayImage);
            gc->DrawBitmap(bitmapToDraw, offset_x, offset_y, final_width, final_height);
            
            // Dibuja la superposición interactiva encima de la imagen
            m_renderer->Draw(gc, *m_interactor, wxPoint2DDouble(offset_x, offset_y), scale_factor);
            
            delete gc;
        }
    }
}

void InputController::OnSizePreview(wxSizeEvent& event)
{
    m_frame->m_rawImagePreviewPanel->Refresh();
    event.Skip();
}

/**
 * @brief Maneja el evento de pulsación del botón izquierdo del ratón en el panel de previsualización.
 */
void InputController::OnPreviewMouseDown(wxMouseEvent& event)
{
    if (!m_rawPreviewImage.IsOk()) return;

    wxPoint2DDouble imageCoords = PanelToImageCoords(event.GetPosition());

    // El radio para el hit-test debe estar en coordenadas de imagen, no de panel.
    double img_w = m_rawPreviewImage.GetWidth();
    double panel_w = m_frame->m_rawImagePreviewPanel->GetSize().GetWidth();
    double scale_factor = img_w > 0 ? panel_w / img_w : 1.0;
    double handleRadiusInImageCoords = 8.0 / scale_factor;

    ChartCornerInteractor::Corner corner = m_interactor->HitTest(wxPoint(imageCoords.m_x, imageCoords.m_y), handleRadiusInImageCoords);

    if (corner != ChartCornerInteractor::Corner::None)
    {
        m_interactor->BeginDrag(corner);
        m_frame->m_rawImagePreviewPanel->CaptureMouse();
        m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_HAND));
    }

    event.Skip();
}

/**
 * @brief Maneja el evento de liberación del botón izquierdo del ratón.
 */
void InputController::OnPreviewMouseUp(wxMouseEvent& event)
{
    if (m_interactor->IsDragging())
    {
        m_interactor->EndDrag();
        if (m_frame->m_rawImagePreviewPanel->HasCapture())
        {
            m_frame->m_rawImagePreviewPanel->ReleaseMouse();
        }
        m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_DEFAULT));

        UpdateCoordTextCtrls();
        m_frame->m_presenter->UpdateCommandPreview();
    }
    event.Skip();
}

/**
 * @brief Maneja el evento de movimiento del ratón.
 */
void InputController::OnPreviewMouseMove(wxMouseEvent& event)
{
    if (m_interactor->IsDragging())
    {
        wxPoint2DDouble imageCoords = PanelToImageCoords(event.GetPosition());
        m_interactor->UpdateDraggedCorner(wxPoint(imageCoords.m_x, imageCoords.m_y));
        m_frame->m_rawImagePreviewPanel->Refresh(); // Fuerza el redibujado
    }
    event.Skip();
}

/**
 * @brief Maneja el evento de pérdida de captura del ratón.
 */
void InputController::OnPreviewMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
    m_interactor->EndDrag();
    m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_DEFAULT));
}

/**
 * @brief Convierte coordenadas del panel a coordenadas relativas a la imagen de previsualización.
 */
wxPoint2DDouble InputController::PanelToImageCoords(const wxPoint& panelPoint) const
{
    if (!m_rawPreviewImage.IsOk()) return wxPoint2DDouble(0,0);

    const wxSize panel_size = m_frame->m_rawImagePreviewPanel->GetSize();
    double img_w = m_rawPreviewImage.GetWidth();
    double img_h = m_rawPreviewImage.GetHeight();

    double scale_factor = std::min((double)panel_size.GetWidth() / img_w, (double)panel_size.GetHeight() / img_h);
    double final_width = img_w * scale_factor;
    double final_height = img_h * scale_factor;
    double offset_x = (panel_size.GetWidth() - final_width) / 2.0;
    double offset_y = (panel_size.GetHeight() - final_height) / 2.0;

    double imageX = (panelPoint.x - offset_x) / scale_factor;
    double imageY = (panelPoint.y - offset_y) / scale_factor;

    return wxPoint2DDouble(imageX, imageY);
}

/**
 * @brief Actualiza los cuadros de texto de coordenadas a partir del estado del interactor.
 */
void InputController::UpdateCoordTextCtrls()
{
    if (!m_rawPreviewImage.IsOk() || m_originalRawWidth == 0) return;

    const auto& corners = m_interactor->GetCorners();
    double scale = static_cast<double>(m_originalRawWidth) / m_rawPreviewImage.GetWidth();

    m_frame->m_coordX1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[0].m_x * scale))));
    m_frame->m_coordY1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[0].m_y * scale))));
    m_frame->m_coordX2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[1].m_x * scale))));
    m_frame->m_coordY2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[1].m_y * scale))));
    m_frame->m_coordX3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[2].m_x * scale))));
    m_frame->m_coordY3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[2].m_y * scale))));
    m_frame->m_coordX4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[3].m_x * scale))));
    m_frame->m_coordY4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(corners[3].m_y * scale))));
}

/**
 * @brief Determina si el nivel de negro debe ser estimado.
 * @details Devuelve true solo si tanto el selector de fichero como el campo de texto están vacíos.
 */
bool InputController::ShouldEstimateBlackLevel() const
{
    bool isFilePickerEmpty = m_frame->m_darkFilePicker->GetPath().IsEmpty();
    bool isTextBoxEmpty = m_frame->m_darkValueTextCtrl->GetValue().IsEmpty();
    return isFilePickerEmpty && isTextBoxEmpty;
}

/**
 * @brief Determina si el nivel de saturación debe ser estimado.
 * @details Devuelve true solo si tanto el selector de fichero como el campo de texto de saturación están vacíos.
 */
bool InputController::ShouldEstimateSaturationLevel() const
{
    bool isFilePickerEmpty = m_frame->m_saturationFilePicker->GetPath().IsEmpty();
    bool isTextBoxEmpty = m_frame->m_saturationValueTextCtrl->GetValue().IsEmpty();
    return isFilePickerEmpty && isTextBoxEmpty;
}