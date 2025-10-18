// File: src/gui/controllers/PreviewController.hpp
/**
 * @file src/gui/controllers/PreviewController.hpp
 * @brief Declares a controller for the interactive RAW preview panel.
 * @details This class adheres to SRP by encapsulating all logic related to the
 * preview image, including user interaction (mouse/keyboard), coordinate
 * transformation, and display adjustments like gamma/contrast.
 */
#pragma once

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
class wxCommandEvent; // For text change event

class PreviewController {
public:
  /**
   * @brief Constructs the PreviewController.
   * @param frame A pointer to the main application frame to bind events and access UI elements.
   */
  explicit PreviewController(DynaRangeFrame *frame);

  /**
   * @brief Loads and displays a new RAW file in the preview panel. Updates corner interactor state and text controls.
   * @param path The filesystem path to the RAW image file. If empty, clears the preview and deactivates manual coordinates.
   */
  void DisplayPreviewImage(const std::string &path);

  /**
   * @brief Resets the interactive corner positions to the image boundaries, deselects any active corner, redraws both the preview and loupe panels, clears text controls and deactivates manual mode.
   */
  void ResetCornerInteraction();

private:
  // --- Event Handling Logic ---
  /**
   * @brief Handles the wxEVT_PAINT event for the main preview panel (m_rawImagePreviewPanel).
   * Draws the gamma-corrected preview image and the interactive overlay (handles, lines).
   * @param event The paint event details.
   */
  void OnPaintPreview(wxPaintEvent &event);
  /**
   * @brief Handles the wxEVT_PAINT event for the dedicated loupe panel (m_loupePanel).
   * Draws the magnified loupe view centered on the currently selected/dragged corner.
   * @param event The paint event details.
   */
  void OnPaintLoupe(wxPaintEvent &event);
  /**
   * @brief Handles the wxEVT_SIZE event for the main preview panel.
   * Recalculates the image scaling and offset based on the new panel size and refreshes the display.
   * @param event The size event details.
   */
  void OnSizePreview(wxSizeEvent &event);
  /**
   * @brief Handles the left mouse button down event on the preview panel.
   * Activates manual coordinate mode if needed, sets focus, performs a hit test for corner handles,
   * initiates drag if a handle is clicked, and refreshes both the preview and loupe panels.
   * @param event The mouse event details.
   */
  void OnPreviewMouseDown(wxMouseEvent &event);
  /**
   * @brief Handles the left mouse button up event on the preview panel.
   * Ends the drag operation if active, releases mouse capture, updates coordinate text controls,
   * updates the command preview, and refreshes the loupe panel.
   * @param event The mouse event details.
   */
  void OnPreviewMouseUp(wxMouseEvent &event);
  /**
   * @brief Handles the mouse motion event on the preview panel.
   * If dragging, updates the dragged corner's position based on mouse coordinates
   * and refreshes both the preview and loupe panels.
   * @param event The mouse event details.
   */
  void OnPreviewMouseMove(wxMouseEvent &event);
  /**
   * @brief Handles the event when mouse capture is lost (e.g., due to Alt+Tab).
   * Ends any active drag operation, resets the cursor, and refreshes both panels.
   * @param event The mouse capture lost event details.
   */
  void OnPreviewMouseCaptureLost(wxMouseCaptureLostEvent &event);
  /**
   * @brief Handles the event when the gamma/contrast slider's value changes.
   * Reapplies the gamma correction to the display image and refreshes both panels.
   * @param event The scroll event details.
   */
  void OnGammaSliderChanged(wxScrollEvent &event);
  /**
   * @brief Handles key down events on the preview panel (specifically arrow keys).
   * Activates manual coordinate mode if needed, moves the currently selected corner handle
   * by one pixel, updates coordinate text controls, updates the command preview, and refreshes both panels.
   * @param event The key event details.
   */
  void OnPreviewKeyDown(wxKeyEvent &event);
  /**
   * @brief Handles the wxEVT_TEXT event for the coordinate text controls.
   * Activates manual coordinate mode if needed, reads the manually entered coordinates,
   * validates them, converts them to preview image coordinates, updates the internal state
   * of the corner interactor, and refreshes the UI.
   * @param event The command event details, used to identify the changed text control.
   */
  void OnCoordTextChanged(wxCommandEvent &event);

  // --- Helper Methods ---
  /**
   * @brief Activates the manual coordinate mode if it wasn't already active.
   * Populates all coordinate text boxes with the current corner positions (transformed to RAW coords)
   * and triggers an update of the command preview. Called upon first user interaction (drag, key move, text edit).
   * @return true if the mode was newly activated, false if it was already active.
   */
  bool ActivateManualCoordinates();
  /**
   * @brief Applies gamma and contrast adjustments to the original preview image to create the display image.
   * Reads the current value from the gamma slider.
   */
  void ApplyGammaCorrection();
  /**
   * @brief Calculates the scaling factor and offset needed to display the preview image centered within the panel while maintaining aspect ratio.
   * Stores the results in m_previewScale and m_previewOffset.
   */
  void UpdatePreviewTransform();
  /**
   * @brief Converts coordinates from the preview panel's coordinate system to the preview image's coordinate system.
   * Takes into account the scaling and offset applied during rendering.
   * @param panelPoint A point within the preview panel (e.g., mouse click coordinates).
   * @return The corresponding point within the preview image's coordinate system. Returns {0,0} if preview is invalid.
   */
  wxPoint2DDouble PanelToImageCoords(const wxPoint &panelPoint) const;
  /**
   * @brief Updates the coordinate text controls (X1, Y1, etc.). If manual mode is active, populates them
   * based on the current state of the ChartCornerInteractor (transformed to RAW coords). If inactive, clears them.
   */
  void UpdateCoordTextCtrls();
  /**
   * @brief Transforms coordinates from the GUI preview image space (potentially rotated relative to RAW) back to the original, unrotated RAW sensor space.
   * @param guiCoords Vector of 4 points in the coordinate system of the preview image (m_originalPreviewImage), assumed scaled to match the rotated RAW active area.
   * @return Vector of 4 points in the original unrotated RAW coordinate system.
   */
  std::vector<wxPoint2DDouble> TransformGuiToRawCoords(const std::vector<wxPoint2DDouble> &guiCoords) const;
  /**
   * @brief Transforms coordinates from the original RAW sensor space to the (potentially rotated and scaled) preview image space.
   * @param rawCoords A point in the original RAW coordinate system.
   * @return The corresponding point in the preview image's coordinate system. Returns {0,0} if preview is invalid.
   */
  wxPoint2DDouble TransformRawToPreviewCoords(wxPoint2DDouble rawCoords) const;

  DynaRangeFrame *m_frame; ///< Pointer to the parent frame (View).

  // --- Preview State Variables ---
  wxImage m_originalPreviewImage; ///< The original, unmodified preview image loaded from the RAW file.
  wxImage m_displayPreviewImage;  ///< The gamma-corrected image that is actually shown on screen.

  int m_originalActiveWidth = 0;  ///< Width of the original RAW active area.
  int m_originalActiveHeight = 0; ///< Height of the original RAW active area.
  int m_rawOrientation = 0;       ///< Orientation flag read from RAW metadata.

  // --- Interaction Components ---
  std::unique_ptr<ChartCornerInteractor> m_interactor; ///< Manages the state of the corner handles.
  std::unique_ptr<PreviewOverlayRenderer> m_renderer; ///< Draws the overlay graphics.

  // --- Transformation Cache ---
  double m_previewScale = 1.0;                  ///< Stores the calculated scale factor for the preview image.
  wxPoint2DDouble m_previewOffset = {0.0, 0.0}; ///< Stores the top-left offset for the centered preview image.
  
  bool m_isUpdatingTextControls = false; ///< Flag to prevent recursion during text control updates.
  bool m_manualCoordsActive = false;     ///< Tracks if manual coordinate mode is active (text boxes filled, -x arg used).
};