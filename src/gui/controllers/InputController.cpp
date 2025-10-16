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
    m_frame->AVG_ChoiceValue->SetSelection(1); // Default is "Full" (index 1)

    // Ensure the gamma slider is disabled by default.
    m_frame->m_gammaThumbSlider->Enable(false);

    // Instanciar el interactor y el renderizador
    m_interactor = std::make_unique<ChartCornerInteractor>();
    m_renderer = std::make_unique<PreviewOverlayRenderer>();

    // Bind the new gamma slider event.
    m_frame->m_gammaThumbSlider->Bind(wxEVT_SCROLL_CHANGED, &InputController::OnGammaSliderChanged, this);

    // Bind events for the preview panel
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_PAINT, &InputController::OnPaintPreview, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_SIZE, &InputController::OnSizePreview, this);
    // Bind new mouse events for interaction
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_DOWN, &InputController::OnPreviewMouseDown, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_UP, &InputController::OnPreviewMouseUp, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOTION, &InputController::OnPreviewMouseMove, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &InputController::OnPreviewMouseCaptureLost, this);
    // Bind the new keyboard event.
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_KEY_DOWN, &InputController::OnPreviewKeyDown, this);
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
    std::vector<wxTextCtrl*> controls = {
        m_frame->m_coordX1Value, m_frame->m_coordY1Value,
        m_frame->m_coordX2Value, m_frame->m_coordY2Value,
        m_frame->m_coordX3Value, m_frame->m_coordY3Value,
        m_frame->m_coordX4Value, m_frame->m_coordY4Value
    };

    for (wxTextCtrl* control : controls) {
        if (control->GetValue().IsEmpty()) {
            return {}; // Return empty if any coordinate is not set
        }
        double val;
        if (!control->GetValue().ToDouble(&val)) {
            return {}; // Return empty on parsing error
        }
        coords.push_back(val);
    }
    return coords;
}

void InputController::DisplayPreviewImage(const std::string& path)
{
    if (path.empty()) {
        m_originalPreviewImage = wxImage(); // Clear original
        m_displayPreviewImage = wxImage();  // Clear display
        m_originalActiveWidth = 0;
        m_originalActiveHeight = 0;
        m_rawOrientation = 0;
    } else {
        RawFile raw_file(path);
        if (raw_file.Load()) {
            m_rawOrientation = raw_file.GetOrientation();
            m_originalActiveWidth = raw_file.GetActiveWidth();
            m_originalActiveHeight = raw_file.GetActiveHeight();
            
            cv::Mat unrotated_mat = raw_file.GetProcessedImage();
            if (unrotated_mat.empty()) {
                m_originalPreviewImage = wxImage();
                wxLogError("Could not get processed image from RAW file: %s", path);
            } else {
                cv::Mat rotated_mat;
                switch (m_rawOrientation) {
                    case 5:
                        cv::rotate(unrotated_mat, rotated_mat, cv::ROTATE_90_COUNTERCLOCKWISE);
                        break;
                    case 6:
                        cv::rotate(unrotated_mat, rotated_mat, cv::ROTATE_90_CLOCKWISE);
                        break;
                    case 3:
                        cv::rotate(unrotated_mat, rotated_mat, cv::ROTATE_180);
                        break;
                    default:
                        rotated_mat = unrotated_mat;
                        break;
                }

                constexpr int MAX_PREVIEW_DIMENSION = 1920;
                cv::Mat preview_mat;
                if (rotated_mat.cols > MAX_PREVIEW_DIMENSION || rotated_mat.rows > MAX_PREVIEW_DIMENSION) {
                    double scale = static_cast<double>(MAX_PREVIEW_DIMENSION) / std::max(rotated_mat.cols, rotated_mat.rows);
                    cv::resize(rotated_mat, preview_mat, cv::Size(), scale, scale, cv::INTER_AREA);
                } else {
                    preview_mat = rotated_mat;
                }
                m_originalPreviewImage = GuiHelpers::CvMatToWxImage(preview_mat);
            }
        } else {
            m_originalPreviewImage = wxImage();
            m_originalActiveWidth = 0;
            m_originalActiveHeight = 0;
            m_rawOrientation = 0;
            wxLogError("Could not load RAW file for preview: %s", path);
        }
    }
    
    m_frame->m_gammaThumbSlider->Enable(m_originalPreviewImage.IsOk());

    if (m_originalPreviewImage.IsOk()) {
        m_interactor->SetImageSize(m_originalPreviewImage.GetSize());
    } else {
        m_interactor->SetImageSize(wxSize(0,0));
    }

    UpdatePreviewTransform();
    ApplyGammaCorrection();
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
    m_interactor->SetSelectedCorner(ChartCornerInteractor::Corner::None); // Deselect corner

    // Fuerza un redibujado del panel de previsualización para mostrar los manejadores en su nueva posición.
    m_frame->m_rawImagePreviewPanel->Refresh();
    // Actualiza el comando CLI equivalente para reflejar que ya no hay coordenadas manuales.
    m_frame->m_presenter->UpdateCommandPreview();
}

void InputController::OnPaintPreview(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(m_frame->m_rawImagePreviewPanel);
    dc.Clear();
    if (m_displayPreviewImage.IsOk()) {
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            double final_width = m_originalPreviewImage.GetWidth() * m_previewScale;
            double final_height = m_originalPreviewImage.GetHeight() * m_previewScale;

            // Create a temporary, scaled copy for drawing in this paint event.
            // This is efficient and ensures the original is untouched.
            wxImage tempDisplayImage = m_displayPreviewImage.Copy();
            tempDisplayImage.Rescale(final_width, final_height, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmapToDraw(tempDisplayImage);
            
            gc->DrawBitmap(bitmapToDraw, m_previewOffset.m_x, m_previewOffset.m_y, final_width, final_height);
            
            // Pass the gamma-corrected (but unscaled) display image to the renderer for the loupe.
            m_renderer->Draw(gc, *m_interactor, m_displayPreviewImage, m_previewOffset, m_previewScale);
            
            delete gc;
        }
    }
}

void InputController::OnSizePreview(wxSizeEvent& event)
{
    // Recalculate transform and refresh.
    UpdatePreviewTransform();
    m_frame->m_rawImagePreviewPanel->Refresh();
    event.Skip();
}

/**
 * @brief Maneja el evento de pulsación del botón izquierdo del ratón en el panel de previsualización.
 */
void InputController::OnPreviewMouseDown(wxMouseEvent& event)
{
    if (!m_originalPreviewImage.IsOk()) return;

    // Set focus to the panel to receive keyboard events.
    m_frame->m_rawImagePreviewPanel->SetFocus();

    wxPoint2DDouble imageCoords = PanelToImageCoords(event.GetPosition());
    
    double handleRadiusInImageCoords = 8.0 / m_previewScale;

    ChartCornerInteractor::Corner corner = m_interactor->HitTest(wxPoint(imageCoords.m_x, imageCoords.m_y), handleRadiusInImageCoords);
    
    m_interactor->SetSelectedCorner(corner);

    if (corner != ChartCornerInteractor::Corner::None)
    {
        m_interactor->BeginDrag(corner);
        m_frame->m_rawImagePreviewPanel->CaptureMouse();
        m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_HAND));
    }

    m_frame->m_rawImagePreviewPanel->Refresh();
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
    if (!m_originalPreviewImage.IsOk() || m_previewScale == 0) return wxPoint2DDouble(0,0);

    double imageX = (panelPoint.x - m_previewOffset.m_x) / m_previewScale;
    double imageY = (panelPoint.y - m_previewOffset.m_y) / m_previewScale;

    return wxPoint2DDouble(imageX, imageY);
}

/**
 * @brief Actualiza los cuadros de texto de coordenadas a partir del estado del interactor.
 */
void InputController::UpdateCoordTextCtrls()
{
    if (!m_originalPreviewImage.IsOk() || m_originalActiveWidth == 0) return;

    // --- SCALING LOGIC ---
    // Get dimensions of the preview and the original image.
    double preview_w = m_originalPreviewImage.GetWidth();
    double preview_h = m_originalPreviewImage.GetHeight();
    double original_w = m_originalActiveWidth;
    double original_h = m_originalActiveHeight;

    // Calculate the scaling factor.
    double scale_x = (preview_w > 0) ? original_w / preview_w : 1.0;
    double scale_y = (preview_h > 0) ? original_h / preview_h : 1.0;

    const auto& corners = m_interactor->GetCorners();
    std::vector<wxPoint2DDouble> gui_coords_for_transform;
    for(const auto& p : corners) {
        gui_coords_for_transform.emplace_back(p.m_x * scale_x, p.m_y * scale_y);
    }
    
    // Transform the full-resolution GUI coordinates to the original RAW coordinate system.
    auto raw_coords_points = TransformGuiToRawCoords(gui_coords_for_transform);

    // Update the text controls with the final, integer-based RAW coordinates.
    m_frame->m_coordX1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[0].m_x))));
    m_frame->m_coordY1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[0].m_y))));
    m_frame->m_coordX2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[1].m_x))));
    m_frame->m_coordY2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[1].m_y))));
    m_frame->m_coordX3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[2].m_x))));
    m_frame->m_coordY3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[2].m_y))));
    m_frame->m_coordX4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[3].m_x))));
    m_frame->m_coordY4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[3].m_y))));
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

/**
 * @brief (Función nueva) Transforma las coordenadas de la GUI (posiblemente rotadas) a las coordenadas del sensor RAW original.
 * @param guiCoords Vector de 4 puntos en el sistema de coordenadas de la imagen de previsualización.
 * @return Vector de 4 puntos en el sistema de coordenadas del RAW sin rotar.
 */
std::vector<wxPoint2DDouble> InputController::TransformGuiToRawCoords(const std::vector<wxPoint2DDouble>& guiCoords) const
{
    if (m_rawOrientation == 0) {
        return guiCoords; // No rotation, return the same coordinates.
    }

    // Use the dimensions of the ORIGINAL UNROTATED ACTIVE AREA for transformation.
    double W_raw = static_cast<double>(m_originalActiveWidth);
    double H_raw = static_cast<double>(m_originalActiveHeight);
    
    std::vector<wxPoint2DDouble> rawCoords;
    rawCoords.reserve(guiCoords.size());

    for (const auto& p : guiCoords) {
        double x_gui = p.m_x;
        double y_gui = p.m_y;

        double x_raw = x_gui;
        double y_raw = y_gui;

        // These are the inverse formulas to map from the rotated preview back to the original unrotated raw space.
        switch (m_rawOrientation) {
            case 5: // Preview was rotated 90 CCW. Its size is H_raw x W_raw.
                x_raw = y_gui;
                y_raw = W_raw - 1.0 - x_gui;
                break;
            case 6: // Preview was rotated 90 CW. Its size is H_raw x W_raw.
                x_raw = H_raw - 1.0 - y_gui;
                y_raw = x_gui;
                break;
            case 3: // Preview was rotated 180. Its size is W_raw x H_raw.
                x_raw = W_raw - 1.0 - x_gui;
                y_raw = H_raw - 1.0 - y_gui;
                break;
        }
        rawCoords.emplace_back(x_raw, y_raw);
    }
    return rawCoords;
}
void InputController::UpdatePreviewTransform()
{
    if (!m_originalPreviewImage.IsOk()) {
        m_previewScale = 1.0;
        m_previewOffset = {0.0, 0.0};
        return;
    }

    const wxSize panel_size = m_frame->m_rawImagePreviewPanel->GetSize();
    double img_w = m_originalPreviewImage.GetWidth();
    double img_h = m_originalPreviewImage.GetHeight();

    constexpr double margin_factor = 0.95;
    double available_width = panel_size.GetWidth() * margin_factor;
    double available_height = panel_size.GetHeight() * margin_factor;

    m_previewScale = std::min(available_width / img_w, available_height / img_h);
    
    double final_width = img_w * m_previewScale;
    double final_height = img_h * m_previewScale;

    m_previewOffset.m_x = (panel_size.GetWidth() - final_width) / 2.0;
    m_previewOffset.m_y = (panel_size.GetHeight() - final_height) / 2.0;
}

void InputController::OnGammaSliderChanged(wxScrollEvent& event)
{
    ApplyGammaCorrection();
    // Trigger a redraw of the panel
    m_frame->m_rawImagePreviewPanel->Refresh();
}

void InputController::ApplyGammaCorrection()
{
    if (!m_originalPreviewImage.IsOk()) return;

    // Map slider value (0-100) to a gamma range (e.g., 3.0 down to 0.2)
    // A slider value of 50 corresponds to a gamma of 1.0 (no change).
    double sliderValue = m_frame->m_gammaThumbSlider->GetValue();
    double gamma = 0.0;
    if (sliderValue < 50) {
        // Range from 3.0 (darker shadows) down to 1.0
        gamma = 3.0 - (sliderValue / 50.0) * 2.0;
    } else {
        // Range from 1.0 down to 0.2 (brighter shadows)
        gamma = 1.0 - ((sliderValue - 50.0) / 50.0) * 0.8;
    }

    // Calculate a contrast factor.
    // Contrast is neutral (1.0) at the center and increases towards the ends (e.g., up to 2.0).
    double contrast = 1.0 + (std::abs(sliderValue - 50.0) / 50.0);

    // Create the Look-Up Table (LUT) combining both effects.
    cv::Mat lut(1, 256, CV_8U);
    unsigned char* p = lut.ptr();
    for (int i = 0; i < 256; ++i) {
        double value = pow(i / 255.0, gamma);
        value = (value - 0.5) * contrast + 0.5;
        p[i] = cv::saturate_cast<uchar>(value * 255.0);
    }

    // Apply the combined LUT to the original preview image.
    cv::Mat src_mat = GuiHelpers::WxImageToCvMat(m_originalPreviewImage);
    if (src_mat.empty()) return;

    cv::Mat dst_mat;
    cv::LUT(src_mat, lut, dst_mat);

    // Convert the result back to wxImage for display.
    // The image is NOT scaled here. It remains at its full preview resolution.
    m_displayPreviewImage = GuiHelpers::CvMatToWxImage(dst_mat);
}

void InputController::OnPreviewKeyDown(wxKeyEvent& event)
{
    if (m_interactor->GetSelectedCorner() == ChartCornerInteractor::Corner::None) {
        event.Skip();
        return;
    }

    int dx = 0;
    int dy = 0;

    switch (event.GetKeyCode()) {
        case WXK_UP:    dy = -1; break;
        case WXK_DOWN:  dy = 1;  break;
        case WXK_LEFT:  dx = -1; break;
        case WXK_RIGHT: dx = 1;  break;
        default:
            event.Skip();
            return;
    }

    m_interactor->MoveSelectedCorner(dx, dy);
    UpdateCoordTextCtrls();
    m_frame->m_presenter->UpdateCommandPreview();
    m_frame->m_rawImagePreviewPanel->Refresh();
}