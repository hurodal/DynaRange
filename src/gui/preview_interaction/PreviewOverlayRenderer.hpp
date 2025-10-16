// File: src/gui/preview_interaction/PreviewOverlayRenderer.hpp
/**
 * @file src/gui/preview_interaction/PreviewOverlayRenderer.hpp
 * @brief Declares a class responsible for rendering the interactive corner overlay.
 * @details This class adheres to SRP by encapsulating all drawing logic for the
 * interactive handles and connecting lines, separating it from state management
 * and event handling. It uses wxGraphicsContext for high-quality, anti-aliased rendering.
 */
#pragma once

#include <wx/graphics.h>
#include <wx/image.h>
#include "ChartCornerInteractor.hpp"

class PreviewOverlayRenderer {
public:
    /**
     * @brief Constructor.
     */
    PreviewOverlayRenderer();

    /**
     * @brief Draws the complete overlay (handles, lines, and loupe) onto a graphics context.
     * @param gc The wxGraphicsContext to draw on.
     * @param interactor The ChartCornerInteractor instance containing the state.
     * @param displayImage The gamma-corrected, unscaled preview image for the loupe.
     * @param imageOffset The top-left offset of the displayed image in the panel.
     * @param imageToPanelScale The scaling factor for coordinates.
     */
    void Draw(
        wxGraphicsContext* gc,
        const ChartCornerInteractor& interactor,
        const wxImage& displayImage,
        const wxPoint2DDouble& imageOffset,
        double imageToPanelScale
    );

private:
    /**
     * @brief Draws the four corner handles (circles).
     * @param gc The wxGraphicsContext to draw on.
     * @param interactor The ChartCornerInteractor providing the corner positions.
     * @param imageOffset The top-left offset of the displayed image in the panel.
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
     * @param imageOffset The top-left offset of the displayed image in the panel.
     * @param imageToPanelScale The scaling factor for coordinates.
     */
    void DrawConnectingLines(
        wxGraphicsContext* gc,
        const ChartCornerInteractor& interactor,
        const wxPoint2DDouble& imageOffset,
        double imageToPanelScale
    );
    /**
     * @brief Draws the magnified loupe view in the top-left corner during a drag.
     * @param gc The wxGraphicsContext to draw on.
     * @param interactor The ChartCornerInteractor providing the dragged corner's position.
     * @param sourceImage The unscaled preview image to source pixels from.
     */
    void DrawLoupe(
        wxGraphicsContext* gc,
        const ChartCornerInteractor& interactor,
        const wxImage& sourceImage
    );
};