// File: src/gui/preview_interaction/PreviewOverlayRenderer.hpp
/**
 * @file PreviewOverlayRenderer.hpp
 * @brief Declares a class responsible for rendering the interactive corner overlay.
 * @details This class adheres to SRP by encapsulating all drawing logic for the
 * interactive handles and connecting lines, separating it from state management
 * and event handling. It uses wxGraphicsContext for high-quality, anti-aliased rendering.
 */
#pragma once

#include <wx/graphics.h>
#include "ChartCornerInteractor.hpp"

class PreviewOverlayRenderer {
public:
    /**
     * @brief Constructor.
     */
    PreviewOverlayRenderer();

    /**
     * @brief Draws the complete overlay (handles and connecting lines) onto a graphics context.
     * @param gc The wxGraphicsContext to draw on.
     * @param interactor The ChartCornerInteractor instance containing the state (corner positions, etc.).
     * @param imageToPanelScale The scaling factor to convert from image coordinates to panel coordinates.
     */
    void Draw(
        wxGraphicsContext* gc,
        const ChartCornerInteractor& interactor,
        const wxPoint2DDouble& imageOffset,
        double imageToPanelScale
    );

private:
    /**
     * @brief Draws the four corner handles (circles).
     * @param gc The wxGraphicsContext to draw on.
     * @param interactor The ChartCornerInteractor providing the corner positions.
     * @param imageToPanelScale The scaling factor for coordinates.
     */
    void DrawHandles(
        wxGraphicsContext* gc,
        const ChartCornerInteractor& interactor,
        const wxPoint2DDouble& imageOffset,
        double imageToPanelScale
    );

    /**
     * @brief Draws the lines connecting the four corner handles.
     * @param gc The wxGraphicsContext to draw on.
     * @param interactor The ChartCornerInteractor providing the corner positions.
     * @param imageToPanelScale The scaling factor for coordinates.
     */
    void DrawConnectingLines(
        wxGraphicsContext* gc,
        const ChartCornerInteractor& interactor,
        const wxPoint2DDouble& imageOffset,
        double imageToPanelScale
    );
};