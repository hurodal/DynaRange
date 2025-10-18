// File: src/gui/controllers/PreviewController.cpp
/**
 * @file src/gui/controllers/PreviewController.cpp
 * @brief Implements the controller for the interactive RAW preview panel.
 */
#include "PreviewController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../GuiPresenter.hpp"
#include "../helpers/CvWxImageConverter.hpp"
#include "../../core/io/raw/RawFile.hpp"
#include <opencv2/imgproc.hpp>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/log.h>
#include <wx/textctrl.h>

PreviewController::PreviewController(DynaRangeFrame *frame)
    : m_frame(frame),
      m_isUpdatingTextControls(false),
      m_manualCoordsActive(false) // Initialize manual mode as inactive
{
  // Instantiate interaction components
  m_interactor = std::make_unique<ChartCornerInteractor>();
  m_renderer = std::make_unique<PreviewOverlayRenderer>();

  // Bind events for the main preview panel
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_PAINT, &PreviewController::OnPaintPreview, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_SIZE, &PreviewController::OnSizePreview, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_DOWN, &PreviewController::OnPreviewMouseDown, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_UP, &PreviewController::OnPreviewMouseUp, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOTION, &PreviewController::OnPreviewMouseMove, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &PreviewController::OnPreviewMouseCaptureLost, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_KEY_DOWN, &PreviewController::OnPreviewKeyDown, this);

  // Bind the paint event for the dedicated loupe panel
  m_frame->m_loupePanel->Bind(wxEVT_PAINT, &PreviewController::OnPaintLoupe, this);

  // Bind gamma slider event
  m_frame->m_gammaThumbSlider->Bind(wxEVT_SCROLL_CHANGED, &PreviewController::OnGammaSliderChanged, this);

  // Bind text change events for all coordinate text controls
  m_frame->m_coordX1Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
  m_frame->m_coordY1Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
  m_frame->m_coordX2Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
  m_frame->m_coordY2Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
  m_frame->m_coordX3Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
  m_frame->m_coordY3Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
  m_frame->m_coordX4Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
  m_frame->m_coordY4Value->Bind(wxEVT_TEXT, &PreviewController::OnCoordTextChanged, this);
}

void PreviewController::DisplayPreviewImage(const std::string &path) {
    if (path.empty()) {
        m_originalPreviewImage = wxImage();
        m_displayPreviewImage = wxImage();
        m_originalActiveWidth = 0;
        m_originalActiveHeight = 0;
        m_rawOrientation = 0;
        m_manualCoordsActive = false; // Deactivate on clear
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
                m_manualCoordsActive = false; // Deactivate on error
            } else {
                cv::Mat rotated_mat;
                switch (m_rawOrientation) {
                case 5: cv::rotate(unrotated_mat, rotated_mat, cv::ROTATE_90_COUNTERCLOCKWISE); break;
                case 6: cv::rotate(unrotated_mat, rotated_mat, cv::ROTATE_90_CLOCKWISE); break;
                case 3: cv::rotate(unrotated_mat, rotated_mat, cv::ROTATE_180); break;
                default: rotated_mat = unrotated_mat; break;
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
                m_manualCoordsActive = false; // Start inactive when loading a new image
            }
        } else {
            m_originalPreviewImage = wxImage();
            m_originalActiveWidth = 0;
            m_originalActiveHeight = 0;
            m_rawOrientation = 0;
            wxLogError("Could not load RAW file for preview: %s", path);
            m_manualCoordsActive = false; // Deactivate on error
        }
    }

    m_frame->m_gammaThumbSlider->Enable(m_originalPreviewImage.IsOk());
    if (m_originalPreviewImage.IsOk()) {
        m_interactor->SetImageSize(m_originalPreviewImage.GetSize());
        // Note: UpdateCoordTextCtrls is called AFTER SetImageSize which might reset corners
    } else {
        m_interactor->SetImageSize(wxSize(0, 0));
    }
    // Update text controls AFTER setting image size and potentially resetting corners.
    // This will clear text boxes because manualCoordsActive is false.
    UpdateCoordTextCtrls();

    UpdatePreviewTransform();
    ApplyGammaCorrection(); // Apply initial gamma/contrast
    m_frame->m_rawImagePreviewPanel->Refresh();
    m_frame->m_loupePanel->Refresh(); // Refresh loupe as well
}


void PreviewController::OnPaintPreview(wxPaintEvent &event) {
    wxAutoBufferedPaintDC dc(m_frame->m_rawImagePreviewPanel);
    dc.Clear();
    if (m_displayPreviewImage.IsOk()) {
        wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
        if (gc) {
            double final_width = m_originalPreviewImage.GetWidth() * m_previewScale;
            double final_height = m_originalPreviewImage.GetHeight() * m_previewScale;

            wxImage tempDisplayImage = m_displayPreviewImage.Copy();
            tempDisplayImage.Rescale(final_width, final_height, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmapToDraw(tempDisplayImage);
            gc->DrawBitmap(bitmapToDraw, m_previewOffset.m_x, m_previewOffset.m_y, final_width, final_height);

            m_renderer->Draw(gc, *m_interactor, m_displayPreviewImage, m_previewOffset, m_previewScale);

            delete gc;
        }
    }
}

void PreviewController::OnPaintLoupe(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(m_frame->m_loupePanel);
    dc.Clear();
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        m_renderer->DrawLoupe(gc, *m_interactor, m_displayPreviewImage, wxPoint(0,0));
        delete gc;
    }
}

void PreviewController::OnSizePreview(wxSizeEvent &event) {
  UpdatePreviewTransform();
  m_frame->m_rawImagePreviewPanel->Refresh();
  event.Skip();
}

void PreviewController::OnPreviewMouseDown(wxMouseEvent &event) {
  if (!m_originalPreviewImage.IsOk())
    return;

  m_frame->m_rawImagePreviewPanel->SetFocus();
  wxPoint2DDouble imageCoords = PanelToImageCoords(event.GetPosition());

  double handleRadiusInImageCoords = 8.0 / m_previewScale;

  ChartCornerInteractor::Corner corner = m_interactor->HitTest(wxPoint(imageCoords.m_x, imageCoords.m_y), handleRadiusInImageCoords);

  m_interactor->SetSelectedCorner(corner);
  if (corner != ChartCornerInteractor::Corner::None) {
    // Activate manual mode when starting to drag
    ActivateManualCoordinates();
    m_interactor->BeginDrag(corner);
    m_frame->m_rawImagePreviewPanel->CaptureMouse();
    m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_HAND));
  } else {
      // If clicking outside handles, deselect
       m_interactor->SetSelectedCorner(ChartCornerInteractor::Corner::None);
  }


  m_frame->m_rawImagePreviewPanel->Refresh();
  m_frame->m_loupePanel->Refresh();
  event.Skip();
}

void PreviewController::OnPreviewMouseUp(wxMouseEvent &event) {
  if (m_interactor->IsDragging()) {
    m_interactor->EndDrag();
    if (m_frame->m_rawImagePreviewPanel->HasCapture()) {
      m_frame->m_rawImagePreviewPanel->ReleaseMouse();
    }
    m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_DEFAULT));

    UpdateCoordTextCtrls();
    m_frame->m_presenter->UpdateCommandPreview();
  }
  // Refresh loupe even if not dragging, in case a corner was just selected/deselected
  m_frame->m_loupePanel->Refresh();
  event.Skip();
}

void PreviewController::OnPreviewMouseMove(wxMouseEvent &event) {
  if (m_interactor->IsDragging()) {
    wxPoint2DDouble imageCoords = PanelToImageCoords(event.GetPosition());
    m_interactor->UpdateDraggedCorner(wxPoint(imageCoords.m_x, imageCoords.m_y));
    m_frame->m_rawImagePreviewPanel->Refresh();
    m_frame->m_loupePanel->Refresh(); // Refresh loupe on move
  }
  event.Skip();
}

void PreviewController::OnPreviewMouseCaptureLost(wxMouseCaptureLostEvent &event) {
    if (m_interactor->IsDragging()) {
        m_interactor->EndDrag();
        m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_DEFAULT));
        // Refresh potentially needed if drag ended abruptly
        m_frame->m_rawImagePreviewPanel->Refresh();
        m_frame->m_loupePanel->Refresh();
    }
}

wxPoint2DDouble PreviewController::PanelToImageCoords(const wxPoint &panelPoint) const {
  if (!m_originalPreviewImage.IsOk() || m_previewScale == 0)
    return wxPoint2DDouble(0, 0);
  double imageX = (panelPoint.x - m_previewOffset.m_x) / m_previewScale;
  double imageY = (panelPoint.y - m_previewOffset.m_y) / m_previewScale;

  return wxPoint2DDouble(imageX, imageY);
}

void PreviewController::UpdateCoordTextCtrls() {
    // If manual mode is not active, clear all controls
    if (!m_manualCoordsActive) {
         // Set flag to prevent OnCoordTextChanged from firing
        m_isUpdatingTextControls = true;
        m_frame->m_coordX1Value->Clear(); m_frame->m_coordY1Value->Clear();
        m_frame->m_coordX2Value->Clear(); m_frame->m_coordY2Value->Clear();
        m_frame->m_coordX3Value->Clear(); m_frame->m_coordY3Value->Clear();
        m_frame->m_coordX4Value->Clear(); m_frame->m_coordY4Value->Clear();
        m_isUpdatingTextControls = false; // Reset flag
        return;
    }

    // --- If manual mode IS active, proceed to update with values ---
    m_isUpdatingTextControls = true; // Set flag

    if (!m_originalPreviewImage.IsOk() || m_originalActiveWidth == 0) {
        // This case should ideally not happen if manualCoordsActive is true,
        // but handle defensively by clearing.
        m_frame->m_coordX1Value->Clear(); m_frame->m_coordY1Value->Clear();
        m_frame->m_coordX2Value->Clear(); m_frame->m_coordY2Value->Clear();
        m_frame->m_coordX3Value->Clear(); m_frame->m_coordY3Value->Clear();
        m_frame->m_coordX4Value->Clear(); m_frame->m_coordY4Value->Clear();
        m_isUpdatingTextControls = false; // Reset flag
        return;
    };

    // Get dimensions of the preview image
    double preview_w = m_originalPreviewImage.GetWidth();
    double preview_h = m_originalPreviewImage.GetHeight();
    // Get dimensions of the original RAW active area
    double original_w = m_originalActiveWidth;
    double original_h = m_originalActiveHeight;
    // Calculate scaling factors
    double scale_x = (preview_w > 0) ? original_w / preview_w : 1.0;
    double scale_y = (preview_h > 0) ? original_h / preview_h : 1.0;

    // Get current corner positions from the interactor (these are in preview image coordinates)
    const auto &corners = m_interactor->GetCorners();
    std::vector<wxPoint2DDouble> gui_coords_for_transform;
    // Scale preview coordinates up to the size of the *rotated* RAW active area frame
    for (const auto &p : corners) {
        gui_coords_for_transform.emplace_back(p.m_x * scale_x, p.m_y * scale_y);
    }

    // Transform these scaled coordinates back to the *original unrotated* RAW coordinate system
    auto raw_coords_points = TransformGuiToRawCoords(gui_coords_for_transform);

    // Update the text controls with the final, integer-based RAW coordinates
    m_frame->m_coordX1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[0].m_x))));
    m_frame->m_coordY1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[0].m_y))));
    m_frame->m_coordX2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[1].m_x))));
    m_frame->m_coordY2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[1].m_y))));
    m_frame->m_coordX3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[2].m_x))));
    m_frame->m_coordY3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[2].m_y))));
    m_frame->m_coordX4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[3].m_x))));
    m_frame->m_coordY4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[3].m_y))));

    // Reset flag after updates are done
    m_isUpdatingTextControls = false;
}

void PreviewController::UpdatePreviewTransform() {
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

void PreviewController::OnGammaSliderChanged(wxScrollEvent &event)
{
    ApplyGammaCorrection();
    m_frame->m_rawImagePreviewPanel->Refresh();
    m_frame->m_loupePanel->Refresh(); // Also refresh the loupe as its source image changed
}

void PreviewController::ApplyGammaCorrection() {
  if (!m_originalPreviewImage.IsOk())
    return;

  double sliderValue = m_frame->m_gammaThumbSlider->GetValue();
  double gamma = 0.0;
  if (sliderValue < 50) {
    gamma = 3.0 - (sliderValue / 50.0) * 2.0;
  } else {
    gamma = 1.0 - ((sliderValue - 50.0) / 50.0) * 0.8;
  }

  double contrast = 1.0 + (std::abs(sliderValue - 50.0) / 50.0);

  cv::Mat lut(1, 256, CV_8U);
  unsigned char *p = lut.ptr();
  for (int i = 0; i < 256; ++i) {
    double value = pow(i / 255.0, gamma);
    value = (value - 0.5) * contrast + 0.5;
    p[i] = cv::saturate_cast<uchar>(value * 255.0);
  }

  cv::Mat src_mat = GuiHelpers::WxImageToCvMat(m_originalPreviewImage);
  if (src_mat.empty())
    return;

  cv::Mat dst_mat;
  cv::LUT(src_mat, lut, dst_mat);

  m_displayPreviewImage = GuiHelpers::CvMatToWxImage(dst_mat);
}

void PreviewController::OnPreviewKeyDown(wxKeyEvent &event)
{
    if (!m_originalPreviewImage.IsOk() || m_interactor->GetSelectedCorner() == ChartCornerInteractor::Corner::None) {
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
            return; // Only handle arrow keys
    }

    // Activate manual mode when moving with keys
    ActivateManualCoordinates();

    m_interactor->MoveSelectedCorner(dx, dy);
    UpdateCoordTextCtrls(); // Update text boxes after moving
    m_frame->m_presenter->UpdateCommandPreview();
    m_frame->m_rawImagePreviewPanel->Refresh();
    m_frame->m_loupePanel->Refresh();
}

void PreviewController::ResetCornerInteraction() {
    m_manualCoordsActive = false; // Deactivate manual mode
    if (m_interactor) {
        m_interactor->ResetCorners();
        m_interactor->SetSelectedCorner(ChartCornerInteractor::Corner::None);
        // Update text controls AFTER resetting internally
        UpdateCoordTextCtrls(); // This will clear the text boxes now
    }
    // Refresh both panels
    m_frame->m_rawImagePreviewPanel->Refresh();
    m_frame->m_loupePanel->Refresh();
    // Update command preview AFTER deactivating
    m_frame->m_presenter->UpdateCommandPreview();
}

void PreviewController::OnCoordTextChanged(wxCommandEvent& event) {
    // If we are currently updating text controls programmatically, ignore this event
    if (m_isUpdatingTextControls || !m_originalPreviewImage.IsOk()) {
        return;
    }

    // Activate manual mode as soon as the user starts typing
    bool newlyActivated = ActivateManualCoordinates();
    // If activation just happened and populated the boxes, we might still want
    // to process the user's input immediately after.

    wxTextCtrl* changedCtrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
    if (!changedCtrl) return;

    ChartCornerInteractor::Corner corner = ChartCornerInteractor::Corner::None;
    wxTextCtrl *xCtrl = nullptr, *yCtrl = nullptr;

    // Determine which corner this text box belongs to
    if (changedCtrl == m_frame->m_coordX1Value || changedCtrl == m_frame->m_coordY1Value) {
        corner = ChartCornerInteractor::Corner::TL; xCtrl = m_frame->m_coordX1Value; yCtrl = m_frame->m_coordY1Value;
    } else if (changedCtrl == m_frame->m_coordX2Value || changedCtrl == m_frame->m_coordY2Value) {
        corner = ChartCornerInteractor::Corner::BL; xCtrl = m_frame->m_coordX2Value; yCtrl = m_frame->m_coordY2Value;
    } else if (changedCtrl == m_frame->m_coordX3Value || changedCtrl == m_frame->m_coordY3Value) {
        corner = ChartCornerInteractor::Corner::BR; xCtrl = m_frame->m_coordX3Value; yCtrl = m_frame->m_coordY3Value;
    } else if (changedCtrl == m_frame->m_coordX4Value || changedCtrl == m_frame->m_coordY4Value) {
        corner = ChartCornerInteractor::Corner::TR; xCtrl = m_frame->m_coordX4Value; yCtrl = m_frame->m_coordY4Value;
    }

    if (corner == ChartCornerInteractor::Corner::None || !xCtrl || !yCtrl) return;

    // Try parsing both X and Y values
    double rawX, rawY;
    // Check if both text boxes contain valid numbers before proceeding
    if (!xCtrl->GetValue().ToDouble(&rawX) || !yCtrl->GetValue().ToDouble(&rawY)) {
        // Invalid input in one or both boxes.
        // Do not update the interactor state yet. Optionally add visual feedback.
        return;
    }

    // Clamp coordinates to original RAW active dimensions
    rawX = std::max(0.0, std::min(rawX, static_cast<double>(m_originalActiveWidth > 0 ? m_originalActiveWidth - 1 : 0)));
    rawY = std::max(0.0, std::min(rawY, static_cast<double>(m_originalActiveHeight > 0 ? m_originalActiveHeight - 1 : 0)));

    // Transform RAW coordinates to Preview coordinates
    wxPoint2DDouble previewCoords = TransformRawToPreviewCoords(wxPoint2DDouble(rawX, rawY));

    // Update the interactor state
    m_interactor->SetCornerPosition(corner, previewCoords);

    // Refresh the UI
    m_frame->m_rawImagePreviewPanel->Refresh();
    m_frame->m_loupePanel->Refresh();
    // Command preview was already updated by ActivateManualCoordinates if needed
    if (!newlyActivated) {
        m_frame->m_presenter->UpdateCommandPreview();
    }


    // Select the corner that was just edited
    m_interactor->SetSelectedCorner(corner);
}

std::vector<wxPoint2DDouble> PreviewController::TransformGuiToRawCoords(const std::vector<wxPoint2DDouble> &guiCoords) const {
    // If there's no rotation, the GUI coordinates (already scaled to match the active area size) are the RAW coordinates.
    if (m_rawOrientation == 0) {
        return guiCoords;
    }

    // Dimensions of the ORIGINAL UNROTATED ACTIVE AREA
    double W_raw = static_cast<double>(m_originalActiveWidth);
    double H_raw = static_cast<double>(m_originalActiveHeight);

    std::vector<wxPoint2DDouble> rawCoords;
    rawCoords.reserve(guiCoords.size());
    // guiCoords are assumed to be scaled to the dimensions of the *rotated* RAW frame
    for (const auto& p : guiCoords) {
        double x_gui_scaled = p.m_x;
        double y_gui_scaled = p.m_y;

        double x_raw = x_gui_scaled;
        double y_raw = y_gui_scaled;

        // Inverse transformations mapping from rotated space back to original unrotated space
        switch (m_rawOrientation) {
            case 5: // Preview was rotated 90 CCW. Its frame size is H_raw x W_raw.
                    // Input coords (x_gui_scaled, y_gui_scaled) are relative to this HxW frame.
                    // Map back to original WxH frame.
                x_raw = y_gui_scaled;
                y_raw = W_raw - 1.0 - x_gui_scaled;
                break;
            case 6: // Preview was rotated 90 CW. Its frame size is H_raw x W_raw.
                    // Input coords (x_gui_scaled, y_gui_scaled) are relative to this HxW frame.
                    // Map back to original WxH frame.
                x_raw = H_raw - 1.0 - y_gui_scaled;
                y_raw = x_gui_scaled;
                break;
            case 3: // Preview was rotated 180. Its frame size is W_raw x H_raw.
                    // Input coords (x_gui_scaled, y_gui_scaled) are relative to this WxH frame.
                    // Map back to original WxH frame.
                x_raw = W_raw - 1.0 - x_gui_scaled;
                y_raw = H_raw - 1.0 - y_gui_scaled;
                break;
            // No default needed as case 0 is handled at the start
        }
        rawCoords.emplace_back(x_raw, y_raw);
    }
    return rawCoords;
}

wxPoint2DDouble PreviewController::TransformRawToPreviewCoords(wxPoint2DDouble rawCoords) const {
    double x_raw = rawCoords.m_x;
    double y_raw = rawCoords.m_y;

    double x_rotated_raw = x_raw;
    double y_rotated_raw = y_raw;

    // Dimensions of the ORIGINAL UNROTATED ACTIVE AREA
    double W_raw = static_cast<double>(m_originalActiveWidth);
    double H_raw = static_cast<double>(m_originalActiveHeight);

    // Apply forward rotation (Raw -> Rotated Raw Frame)
    switch (m_rawOrientation) {
        case 5: // Rotated 90 CCW. New frame size is H_raw x W_raw.
            x_rotated_raw = W_raw - 1.0 - y_raw;
            y_rotated_raw = x_raw;
            break;
        case 6: // Rotated 90 CW. New frame size is H_raw x W_raw.
            x_rotated_raw = y_raw;
            y_rotated_raw = H_raw - 1.0 - x_raw;
            break;
        case 3: // Rotated 180. New frame size is W_raw x H_raw.
            x_rotated_raw = W_raw - 1.0 - x_raw;
            y_rotated_raw = H_raw - 1.0 - y_raw;
            break;
        case 0: // No rotation
        default:
            // Coordinates remain the same, frame size is W_raw x H_raw
            break;
    }

    // Apply scaling (Rotated Raw Frame -> Preview Image Frame)
    if (!m_originalPreviewImage.IsOk() || m_originalActiveWidth == 0 || m_originalActiveHeight == 0) {
        return wxPoint2DDouble(0,0); // Cannot scale if dimensions are invalid
    }
    // Dimensions of the preview image (which matches the rotated raw frame's aspect ratio)
    double preview_w = m_originalPreviewImage.GetWidth();
    double preview_h = m_originalPreviewImage.GetHeight();
    
    // Determine the effective raw dimensions AFTER rotation
    double rotated_raw_w = (m_rawOrientation == 5 || m_rawOrientation == 6) ? H_raw : W_raw;
    double rotated_raw_h = (m_rawOrientation == 5 || m_rawOrientation == 6) ? W_raw : H_raw;

    // Calculate scaling factors from rotated raw to preview
    double scale_x = (rotated_raw_w > 0) ? preview_w / rotated_raw_w : 1.0;
    double scale_y = (rotated_raw_h > 0) ? preview_h / rotated_raw_h : 1.0;

    // Scale the rotated raw coordinates to get preview coordinates
    double previewX = x_rotated_raw * scale_x;
    double previewY = y_rotated_raw * scale_y;

    return wxPoint2DDouble(previewX, previewY);
}

bool PreviewController::ActivateManualCoordinates() {
    if (!m_manualCoordsActive) {
        m_manualCoordsActive = true;
        // Populate text controls with current visual positions
        // UpdateCoordTextCtrls handles the transformation and setting values
        UpdateCoordTextCtrls();
        // Update command preview as coordinates are now active
        m_frame->m_presenter->UpdateCommandPreview();
        return true; // Indicate that activation just happened
    }
    return false; // Was already active
}