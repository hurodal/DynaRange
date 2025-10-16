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
    const wxImage& displayImage,
    const wxPoint2DDouble& imageOffset,
    double imageToPanelScale)
{
    if (!gc) {
        return;
    }

    // The drawing is done in passes to ensure correct layering.
    DrawConnectingLines(gc, interactor, imageOffset, imageToPanelScale);
    DrawHandles(gc, interactor, imageOffset, imageToPanelScale);

    // If a corner is selected (for keyboard) or being dragged (for mouse), draw the loupe.
    if (interactor.GetSelectedCorner() != ChartCornerInteractor::Corner::None) {
        DrawLoupe(gc, interactor, displayImage);
    }
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

    ChartCornerInteractor::Corner selectedCorner = interactor.GetSelectedCorner();

    for (size_t i = 0; i < corners.size(); ++i)
    {
        const auto& corner = corners[i];
        
        if (static_cast<ChartCornerInteractor::Corner>(i) == selectedCorner) {
            // Use yellow for the selected handle
            gc->SetPen(wxPen(wxColour(PlotColors::YELLOW[0] * 255, PlotColors::YELLOW[1] * 255, PlotColors::YELLOW[2] * 255), HANDLE_BORDER_THICKNESS));
        } else {
            // Use red for unselected handles
            gc->SetPen(wxPen(wxColour(PlotColors::RED[0] * 255, PlotColors::RED[1] * 255, PlotColors::RED[2] * 255), HANDLE_BORDER_THICKNESS));
        }
        
        gc->SetBrush(wxBrush(wxColour(255, 255, 255, 128)));

        // Calculate the center of the handle in panel coordinates.
        double centerX = imageOffset.m_x + corner.m_x * imageToPanelScale;
        double centerY = imageOffset.m_y + corner.m_y * imageToPanelScale;
        
        // Draw the circle for the handle.
        gc->DrawEllipse(centerX - HANDLE_RADIUS, centerY - HANDLE_RADIUS, HANDLE_RADIUS * 2, HANDLE_RADIUS * 2);
    }
}

void PreviewOverlayRenderer::DrawLoupe(
    wxGraphicsContext* gc,
    const ChartCornerInteractor& interactor,
    const wxImage& sourceImage)
{
    if (!sourceImage.IsOk()) return;

    // Determine which corner to magnify.
    ChartCornerInteractor::Corner cornerToMagnify = interactor.GetSelectedCorner();
    if (cornerToMagnify == ChartCornerInteractor::Corner::None) return;

    // Get the position of the active corner.
    wxPoint2DDouble activeCornerPos = interactor.GetCorners()[static_cast<int>(cornerToMagnify)];

    // --- Loupe Configuration ---
    const int loupeSize = 150; // Pixel dimensions of the loupe on screen
    const int magnification = 4;
    const int crosshairSize = 10;
    const wxPoint loupePosition(10, 10); // Top-left corner with a small margin

    // --- Source Area Calculation ---
    const int sourceWidth = loupeSize / magnification;
    const int sourceHeight = loupeSize / magnification;

    // Calculate the top-left corner of the source rectangle in the image
    int sourceX = static_cast<int>(activeCornerPos.m_x - sourceWidth / 2);
    int sourceY = static_cast<int>(activeCornerPos.m_y - sourceHeight / 2);

    // Clamp the source rectangle to stay within the image boundaries
    sourceX = std::max(0, std::min(sourceX, sourceImage.GetWidth() - sourceWidth));
    sourceY = std::max(0, std::min(sourceY, sourceImage.GetHeight() - sourceHeight));
    
    wxRect sourceRect(sourceX, sourceY, sourceWidth, sourceHeight);
    
    // --- Image Extraction and Drawing ---
    wxImage subImage = sourceImage.GetSubImage(sourceRect);
    subImage.Rescale(loupeSize, loupeSize, wxIMAGE_QUALITY_NEAREST); // Use NEAREST for a pixelated look
    wxBitmap loupeBitmap(subImage);

    // Draw the magnified image and its border
    gc->DrawBitmap(loupeBitmap, loupePosition.x, loupePosition.y, loupeSize, loupeSize);
    gc->SetPen(wxPen(*wxBLACK, 2));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(loupePosition.x, loupePosition.y, loupeSize, loupeSize);

    // --- Crosshair Drawing ---
    gc->SetPen(wxPen(*wxRED, 1));
    const int centerX = loupePosition.x + loupeSize / 2;
    const int centerY = loupePosition.y + loupeSize / 2;
    gc->StrokeLine(centerX, centerY - crosshairSize, centerX, centerY + crosshairSize);
    gc->StrokeLine(centerX - crosshairSize, centerY, centerX + crosshairSize, centerY);
}