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
    constexpr int LOUPE_SIZE = 150; // Pixel dimensions of the loupe on screen.
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
    gc->SetPen(wxPen(wxColour(PlotColors::BLUE[0] * 255, PlotColors::BLUE[1] * 255, PlotColors::BLUE[2] * 255), LINE_THICKNESS));
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
            gc->SetPen(wxPen(wxColour(PlotColors::YELLOW[0] * 255, PlotColors::YELLOW[1] * 255, PlotColors::YELLOW[2] * 255), HANDLE_BORDER_THICKNESS));
        } else {
            gc->SetPen(wxPen(wxColour(PlotColors::RED[0] * 255, PlotColors::RED[1] * 255, PlotColors::RED[2] * 255), HANDLE_BORDER_THICKNESS));
        }
        gc->SetBrush(wxBrush(wxColour(255, 255, 255, 128)));
        double centerX = imageOffset.m_x + corner.m_x * imageToPanelScale;
        double centerY = imageOffset.m_y + corner.m_y * imageToPanelScale;
        gc->DrawEllipse(centerX - HANDLE_RADIUS, centerY - HANDLE_RADIUS, HANDLE_RADIUS * 2, HANDLE_RADIUS * 2);
    }
}

void PreviewOverlayRenderer::DrawLoupe(
    wxGraphicsContext* gc,
    const ChartCornerInteractor& interactor,
    const wxImage& sourceImage,
    const wxPoint& loupePosition)
{
    if (!sourceImage.IsOk()) return;
    ChartCornerInteractor::Corner cornerToMagnify = interactor.GetSelectedCorner();
    if (cornerToMagnify == ChartCornerInteractor::Corner::None) return;
    wxPoint2DDouble activeCornerPos = interactor.GetCorners()[static_cast<int>(cornerToMagnify)];
    const int magnification = 4;
    const int crosshairSize = 10;
    const int sourceWidth = LOUPE_SIZE / magnification;
    const int sourceHeight = LOUPE_SIZE / magnification;
    int sourceX = static_cast<int>(activeCornerPos.m_x - sourceWidth / 2);
    int sourceY = static_cast<int>(activeCornerPos.m_y - sourceHeight / 2);
    sourceX = std::max(0, std::min(sourceX, sourceImage.GetWidth() - sourceWidth));
    sourceY = std::max(0, std::min(sourceY, sourceImage.GetHeight() - sourceHeight));
    wxRect sourceRect(sourceX, sourceY, sourceWidth, sourceHeight);
    wxImage subImage = sourceImage.GetSubImage(sourceRect);
    subImage.Rescale(LOUPE_SIZE, LOUPE_SIZE, wxIMAGE_QUALITY_NEAREST);
    wxBitmap loupeBitmap(subImage);
    gc->DrawBitmap(loupeBitmap, loupePosition.x, loupePosition.y, LOUPE_SIZE, LOUPE_SIZE);
    gc->SetPen(wxPen(*wxBLACK, 2));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(loupePosition.x, loupePosition.y, LOUPE_SIZE, LOUPE_SIZE);
    gc->SetPen(wxPen(*wxRED, 1));
    const int centerX = loupePosition.x + LOUPE_SIZE / 2;
    const int centerY = loupePosition.y + LOUPE_SIZE / 2;
    gc->StrokeLine(centerX, centerY - crosshairSize, centerX, centerY + crosshairSize);
    gc->StrokeLine(centerX - crosshairSize, centerY, centerX + crosshairSize, centerY);
}