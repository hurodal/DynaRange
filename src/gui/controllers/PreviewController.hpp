// File: src/gui/controllers/PreviewController.hpp
/**
 * @file src/gui/controllers/PreviewController.hpp
 * @brief Declares a controller for the interactive RAW preview panel.
 * @details This class adheres to SRP by encapsulating all logic related to the
 * preview image, including user interaction (mouse/keyboard), coordinate
 * transformation, and display adjustments like gamma/contrast.
 */
#pragma once

#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../preview_interaction/ChartCornerInteractor.hpp"
#include "../preview_interaction/PreviewOverlayRenderer.hpp"
#include "wx/image.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class DynaRangeFrame;
class wxCommandEvent;
class wxKeyEvent;
class wxMouseEvent;
class wxMouseCaptureLostEvent;
class wxPaintEvent;
class wxScrollEvent;
class wxSizeEvent;

class PreviewController {
public:
  /**
   * @brief Constructs the PreviewController.
   * @param frame A pointer to the main application frame to bind events and access UI elements.
   */
  explicit PreviewController(DynaRangeFrame *frame);

  /**
   * @brief Loads and displays a new RAW file in the preview panel.
   * @param path The filesystem path to the RAW image file.
   */
  void DisplayPreviewImage(const std::string &path);

private:
  // --- Event Handling Logic ---
  void OnPaintPreview(wxPaintEvent &event);
  void OnSizePreview(wxSizeEvent &event);
  void OnPreviewMouseDown(wxMouseEvent &event);
  void OnPreviewMouseUp(wxMouseEvent &event);
  void OnPreviewMouseMove(wxMouseEvent &event);
  void OnPreviewMouseCaptureLost(wxMouseCaptureLostEvent &event);
  void OnGammaSliderChanged(wxScrollEvent &event);
  void OnPreviewKeyDown(wxKeyEvent &event);

  // --- Helper Methods ---
  void ApplyGammaCorrection();
  void UpdatePreviewTransform();
  wxPoint2DDouble PanelToImageCoords(const wxPoint &panelPoint) const;
  void UpdateCoordTextCtrls();
  std::vector<wxPoint2DDouble> TransformGuiToRawCoords(const std::vector<wxPoint2DDouble> &guiCoords) const;

  DynaRangeFrame *m_frame; ///< Pointer to the parent frame (View).

  // --- Preview State Variables ---
  wxImage m_originalPreviewImage; ///< The original, unmodified preview image loaded from the RAW file.
  wxImage m_displayPreviewImage;  ///< The gamma-corrected image that is actually shown on screen.

  int m_originalActiveWidth = 0;
  int m_originalActiveHeight = 0;
  int m_rawOrientation = 0;

  // --- Interaction Components ---
  std::unique_ptr<ChartCornerInteractor> m_interactor;
  std::unique_ptr<PreviewOverlayRenderer> m_renderer;

  // --- Transformation Cache ---
  double m_previewScale = 1.0;                  ///< Stores the calculated scale factor for the preview image.
  wxPoint2DDouble m_previewOffset = {0.0, 0.0}; ///< Stores the top-left offset for the centered preview image.
};