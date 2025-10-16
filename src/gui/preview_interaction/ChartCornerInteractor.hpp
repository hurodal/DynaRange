// File: src/gui/preview_interaction/ChartCornerInteractor.hpp
/**
 * @file src/gui/preview_interaction/ChartCornerInteractor.hpp
 * @brief Declares a class to manage the state and logic of interactive corner handles on a preview image.
 * @details This class adheres to SRP by encapsulating all state (corner positions, drag status)
 * and business logic (hit testing, movement constraints) for the interactive overlay,
 * separating it from the view's event handling and rendering code.
 */
#pragma once

#include <vector>
#include <wx/gdicmn.h>
#include <wx/geometry.h>

class ChartCornerInteractor {
public:
    /**
     * @enum Corner
     * @brief Identifies the four corner handles and a state for no selection.
     */
    enum class Corner { TL, BL, BR, TR, None };

    /**
     * @brief Constructor.
     */
    ChartCornerInteractor();

    /**
     * @brief Sets the dimensions of the underlying image to calculate corner constraints.
     * @param imageSize The full size of the RAW preview image.
     */
    void SetImageSize(const wxSize& imageSize);

    /**
     * @brief Resets the corner positions to the default (the four corners of the image).
     */
    void ResetCorners();

    /**
     * @brief Performs a hit test to determine if a point is over a corner handle.
     * @param point The coordinates of the mouse click in panel coordinates.
     * @param handleRadius The visual radius of the handle for hit detection.
     * @return The identifier of the corner that was hit, or Corner::None.
     */
    Corner HitTest(const wxPoint& point, double handleRadius) const;

    /**
     * @brief Starts a drag operation on a specific corner.
     * @param corner The corner handle to drag.
     */
    void BeginDrag(Corner corner);

    /**
     * @brief Ends the current drag operation.
     */
    void EndDrag();

    /**
     * @brief Checks if a drag operation is currently in progress.
     * @return true if a corner is being dragged, false otherwise.
     */
    bool IsDragging() const;

    /**
     * @brief Updates the position of the currently dragged corner.
     * @details This method enforces the quadrant movement constraints.
     * @param point The new coordinates of the mouse cursor.
     */
    void UpdateDraggedCorner(const wxPoint& point);

    /**
     * @brief Gets the current positions of the four corner handles.
     * @return A constant reference to the vector of corner points.
     */
    const std::vector<wxPoint2DDouble>& GetCorners() const;

    /**
     * @brief Sets the currently selected corner for keyboard interaction.
     * @param corner The corner to select, or Corner::None to deselect.
     */
    void SetSelectedCorner(Corner corner);

    /**
     * @brief Gets the currently selected corner.
     * @return The identifier of the selected corner, or Corner::None.
     */
    Corner GetSelectedCorner() const;

    /**
     * @brief Moves the currently selected corner by a given delta.
     * @param dx The change in the x-coordinate.
     * @param dy The change in the y-coordinate.
     */
    void MoveSelectedCorner(int dx, int dy);

private:
    /**
     * @brief Calculates the bounding rectangle for a specific corner's quadrant.
     * @param corner The corner for which to get the quadrant.
     * @return A wxRect2DDouble representing the allowed movement area.
     */
    wxRect2DDouble GetQuadrant(Corner corner) const;

    /// The dimensions of the source image, used for constraints.
    wxSize m_imageSize;
    /// The current coordinates of the four corners: TL, BL, BR, TR.
    std::vector<wxPoint2DDouble> m_corners;
    /// Flag indicating if a drag operation is active.
    bool m_isDragging;
    /// The identifier of the corner currently being dragged.
    Corner m_draggedCorner;
    /// The identifier of the corner selected for keyboard interaction.
    Corner m_selectedCorner;
};