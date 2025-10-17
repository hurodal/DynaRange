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

PreviewController::PreviewController(DynaRangeFrame *frame) : m_frame(frame) {
  // Instantiate interaction components
  m_interactor = std::make_unique<ChartCornerInteractor>();
  m_renderer = std::make_unique<PreviewOverlayRenderer>();

  // Bind events for the preview panel
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_PAINT, &PreviewController::OnPaintPreview, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_SIZE, &PreviewController::OnSizePreview, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_DOWN, &PreviewController::OnPreviewMouseDown, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_LEFT_UP, &PreviewController::OnPreviewMouseUp, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOTION, &PreviewController::OnPreviewMouseMove, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_MOUSE_CAPTURE_LOST, &PreviewController::OnPreviewMouseCaptureLost, this);
  m_frame->m_rawImagePreviewPanel->Bind(wxEVT_KEY_DOWN, &PreviewController::OnPreviewKeyDown, this);

  // Bind gamma slider event
  m_frame->m_gammaThumbSlider->Bind(wxEVT_SCROLL_CHANGED, &PreviewController::OnGammaSliderChanged, this);
}

void PreviewController::DisplayPreviewImage(const std::string &path) {
  if (path.empty()) {
    m_originalPreviewImage = wxImage();
    m_displayPreviewImage = wxImage();
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
    m_interactor->SetImageSize(wxSize(0, 0));
  }

  UpdatePreviewTransform();
  ApplyGammaCorrection();
  m_frame->m_rawImagePreviewPanel->Refresh();
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
    m_interactor->BeginDrag(corner);
    m_frame->m_rawImagePreviewPanel->CaptureMouse();
    m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_HAND));
  }

  m_frame->m_rawImagePreviewPanel->Refresh();
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
  event.Skip();
}

void PreviewController::OnPreviewMouseMove(wxMouseEvent &event) {
  if (m_interactor->IsDragging()) {
    wxPoint2DDouble imageCoords = PanelToImageCoords(event.GetPosition());
    m_interactor->UpdateDraggedCorner(wxPoint(imageCoords.m_x, imageCoords.m_y));
    m_frame->m_rawImagePreviewPanel->Refresh();
  }
  event.Skip();
}

void PreviewController::OnPreviewMouseCaptureLost(wxMouseCaptureLostEvent &event) {
  m_interactor->EndDrag();
  m_frame->m_rawImagePreviewPanel->SetCursor(wxCursor(wxCURSOR_DEFAULT));
}

wxPoint2DDouble PreviewController::PanelToImageCoords(const wxPoint &panelPoint) const {
  if (!m_originalPreviewImage.IsOk() || m_previewScale == 0)
    return wxPoint2DDouble(0, 0);
  double imageX = (panelPoint.x - m_previewOffset.m_x) / m_previewScale;
  double imageY = (panelPoint.y - m_previewOffset.m_y) / m_previewScale;

  return wxPoint2DDouble(imageX, imageY);
}

void PreviewController::UpdateCoordTextCtrls() {
  if (!m_originalPreviewImage.IsOk() || m_originalActiveWidth == 0)
    return;
  double preview_w = m_originalPreviewImage.GetWidth();
  double preview_h = m_originalPreviewImage.GetHeight();
  double original_w = m_originalActiveWidth;
  double original_h = m_originalActiveHeight;
  double scale_x = (preview_w > 0) ? original_w / preview_w : 1.0;
  double scale_y = (preview_h > 0) ? original_h / preview_h : 1.0;

  const auto &corners = m_interactor->GetCorners();
  std::vector<wxPoint2DDouble> gui_coords_for_transform;
  for (const auto &p : corners) {
    gui_coords_for_transform.emplace_back(p.m_x * scale_x, p.m_y * scale_y);
  }

  auto raw_coords_points = TransformGuiToRawCoords(gui_coords_for_transform);

  m_frame->m_coordX1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[0].m_x))));
  m_frame->m_coordY1Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[0].m_y))));
  m_frame->m_coordX2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[1].m_x))));
  m_frame->m_coordY2Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[1].m_y))));
  m_frame->m_coordX3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[2].m_x))));
  m_frame->m_coordY3Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[2].m_y))));
  m_frame->m_coordX4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[3].m_x))));
  m_frame->m_coordY4Value->ChangeValue(wxString::Format("%d", static_cast<int>(round(raw_coords_points[3].m_y))));
}

std::vector<wxPoint2DDouble> PreviewController::TransformGuiToRawCoords(const std::vector<wxPoint2DDouble> &guiCoords) const {
  if (m_rawOrientation == 0) {
    return guiCoords;
  }

  double W_raw = static_cast<double>(m_originalActiveWidth);
  double H_raw = static_cast<double>(m_originalActiveHeight);

  std::vector<wxPoint2DDouble> rawCoords;
  rawCoords.reserve(guiCoords.size());
  for (const auto &p : guiCoords) {
    double x_gui = p.m_x;
    double y_gui = p.m_y;

    double x_raw = x_gui;
    double y_raw = y_gui;
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

void PreviewController::OnGammaSliderChanged(wxScrollEvent &event) {
  ApplyGammaCorrection();
  m_frame->m_rawImagePreviewPanel->Refresh();
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

void PreviewController::OnPreviewKeyDown(wxKeyEvent &event) {
  if (m_interactor->GetSelectedCorner() == ChartCornerInteractor::Corner::None) {
    event.Skip();
    return;
  }

  int dx = 0;
  int dy = 0;
  switch (event.GetKeyCode()) {
  case WXK_UP:
    dy = -1;
    break;
  case WXK_DOWN:
    dy = 1;
    break;
  case WXK_LEFT:
    dx = -1;
    break;
  case WXK_RIGHT:
    dx = 1;
    break;
  default:
    event.Skip();
    return;
  }

  m_interactor->MoveSelectedCorner(dx, dy);
  UpdateCoordTextCtrls();
  m_frame->m_presenter->UpdateCommandPreview();
  m_frame->m_rawImagePreviewPanel->Refresh();
}