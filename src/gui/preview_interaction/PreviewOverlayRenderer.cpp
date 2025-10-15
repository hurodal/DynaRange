// File: src/gui/preview_interaction/PreviewOverlayRenderer.cpp
/**
 * @file PreviewOverlayRenderer.cpp
 * @brief Implements the PreviewOverlayRenderer class.
 */
#include "PreviewOverlayRenderer.hpp"
#include "../../core/graphics/Colour.hpp"
#include "wx/brush.h"
#include "wx/pen.h"

namespace {
    // Defines the visual appearance of the interactive overlay elements.
    constexpr double HANDLE_RADIUS = 8.0; // Visual radius of the circular handles in pixels.
    constexpr double LINE_THICKNESS = 2.0;
    constexpr double HANDLE_BORDER_THICKNESS = 2.0;
}

PreviewOverlayRenderer::PreviewOverlayRenderer() {}

void PreviewOverlayRenderer::Draw(
    wxGraphicsContext* gc,
    const ChartCornerInteractor& interactor,
    const wxPoint2DDouble& imageOffset,
    double imageToPanelScale)
{
    if (!gc) {
        return;
    }

    // The drawing is done in two passes to ensure lines are behind handles.
    DrawConnectingLines(gc, interactor, imageOffset, imageToPanelScale);
    DrawHandles(gc, interactor, imageOffset, imageToPanelScale);
}

void PreviewOverlayRenderer::DrawConnectingLines(
    wxGraphicsContext* gc,
    const ChartCornerInteractor& interactor,
    const wxPoint2DDouble& imageOffset,
    double imageToPanelScale)
{
    const auto& corners = interactor.GetCorners();
    if (corners.size() != 4) {
        return;
    }

    // Set line style. Blue is chosen for good visibility against most images.
    gc->SetPen(wxPen(wxColour(PlotColors::BLUE[0] * 255, PlotColors::BLUE[1] * 255, PlotColors::BLUE[2] * 255), LINE_THICKNESS));

    // Create a path that connects all four corners and closes the loop.
    wxGraphicsPath path = gc->CreatePath();
    wxPoint2DDouble firstPoint = corners[0];
    path.MoveToPoint(imageOffset.m_x + firstPoint.m_x * imageToPanelScale, imageOffset.m_y + firstPoint.m_y * imageToPanelScale);

    for (size_t i = 1; i < corners.size(); ++i)
    {
        wxPoint2DDouble point = corners[i];
        path.AddLineToPoint(imageOffset.m_x + point.m_x * imageToPanelScale, imageOffset.m_y + point.m_y * imageToPanelScale);
    }
    path.CloseSubpath();

    gc->StrokePath(path);
}

void PreviewOverlayRenderer::DrawHandles(
    wxGraphicsContext* gc,
    const ChartCornerInteractor& interactor,
    const wxPoint2DDouble& imageOffset,
    double imageToPanelScale)
{
    const auto& corners = interactor.GetCorners();
    if (corners.size() != 4) {
        return;
    }

    // Set handle border style. Red is used for high visibility.
    gc->SetPen(wxPen(wxColour(PlotColors::RED[0] * 255, PlotColors::RED[1] * 255, PlotColors::RED[2] * 255), HANDLE_BORDER_THICKNESS));
    // Set handle fill style. A semi-transparent white is used.
    gc->SetBrush(wxBrush(wxColour(255, 255, 255, 128)));

    for (const auto& corner : corners)
    {
        // Calculate the center of the handle in panel coordinates.
        double centerX = imageOffset.m_x + corner.m_x * imageToPanelScale;
        double centerY = imageOffset.m_y + corner.m_y * imageToPanelScale;
        
        // Draw the circle for the handle.
        gc->DrawEllipse(centerX - HANDLE_RADIUS, centerY - HANDLE_RADIUS, HANDLE_RADIUS * 2, HANDLE_RADIUS * 2);
    }
}