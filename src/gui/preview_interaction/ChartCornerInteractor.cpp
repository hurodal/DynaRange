// File: src/gui/preview_interaction/ChartCornerInteractor.cpp
/**
 * @file ChartCornerInteractor.cpp
 * @brief Implements the ChartCornerInteractor class.
 */
#include "ChartCornerInteractor.hpp"
#include <algorithm> // For std::min/max

ChartCornerInteractor::ChartCornerInteractor()
    : m_imageSize(0, 0),
      m_isDragging(false),
      m_draggedCorner(Corner::None),
      m_selectedCorner(Corner::None)
{
    // Initialize with 4 points, which will be properly set by SetImageSize/ResetCorners.
    m_corners.resize(4);
}

void ChartCornerInteractor::SetImageSize(const wxSize& imageSize)
{
    m_imageSize = imageSize;
    ResetCorners();
}

void ChartCornerInteractor::ResetCorners()
{
    // Reset corners to the exact image boundaries.
    m_corners[static_cast<int>(Corner::TL)] = wxPoint2DDouble(0.0, 0.0);
    m_corners[static_cast<int>(Corner::BL)] = wxPoint2DDouble(0.0, m_imageSize.GetHeight() > 0 ? m_imageSize.GetHeight() - 1.0 : 0.0);
    m_corners[static_cast<int>(Corner::BR)] = wxPoint2DDouble(m_imageSize.GetWidth() > 0 ? m_imageSize.GetWidth() - 1.0 : 0.0, m_imageSize.GetHeight() > 0 ? m_imageSize.GetHeight() - 1.0 : 0.0);
    m_corners[static_cast<int>(Corner::TR)] = wxPoint2DDouble(m_imageSize.GetWidth() > 0 ? m_imageSize.GetWidth() - 1.0 : 0.0, 0.0);
}

ChartCornerInteractor::Corner ChartCornerInteractor::HitTest(const wxPoint& point, double handleRadius) const
{
    for (int i = 0; i < 4; ++i)
    {
        wxPoint2DDouble corner_point = m_corners[i];
        double distance = corner_point.GetDistance(wxPoint2DDouble(point.x, point.y));
        if (distance <= handleRadius)
        {
            return static_cast<Corner>(i);
        }
    }
    return Corner::None;
}

void ChartCornerInteractor::BeginDrag(Corner corner)
{
    if (corner != Corner::None)
    {
        m_isDragging = true;
        m_draggedCorner = corner;
    }
}

void ChartCornerInteractor::EndDrag()
{
    m_isDragging = false;
    m_draggedCorner = Corner::None;
}

bool ChartCornerInteractor::IsDragging() const
{
    return m_isDragging;
}

void ChartCornerInteractor::UpdateDraggedCorner(const wxPoint& point)
{
    if (!m_isDragging || m_draggedCorner == Corner::None)
    {
        return;
    }

    wxRect2DDouble quadrant = GetQuadrant(m_draggedCorner);
    
    // Clamp the point to stay within the allowed quadrant and image boundaries
    double newX = std::max(0.0, std::min(static_cast<double>(m_imageSize.GetWidth() - 1), std::max(quadrant.m_x, std::min(static_cast<double>(point.x), quadrant.m_x + quadrant.m_width))));
    double newY = std::max(0.0, std::min(static_cast<double>(m_imageSize.GetHeight() - 1), std::max(quadrant.m_y, std::min(static_cast<double>(point.y), quadrant.m_y + quadrant.m_height))));
    
    m_corners[static_cast<int>(m_draggedCorner)] = wxPoint2DDouble(newX, newY);
}

const std::vector<wxPoint2DDouble>& ChartCornerInteractor::GetCorners() const
{
    return m_corners;
}

wxRect2DDouble ChartCornerInteractor::GetQuadrant(Corner corner) const
{
    double halfWidth = m_imageSize.GetWidth() / 2.0;
    double halfHeight = m_imageSize.GetHeight() / 2.0;
    // Ensure width/height are at least 1 to avoid invalid rectangles if image size is tiny
    double w = std::max(1.0, halfWidth);
    double h = std::max(1.0, halfHeight);

    switch (corner)
    {
        case Corner::TL: return wxRect2DDouble(0, 0, w, h);
        case Corner::BL: return wxRect2DDouble(0, halfHeight, w, h);
        case Corner::BR: return wxRect2DDouble(halfWidth, halfHeight, w, h);
        case Corner::TR: return wxRect2DDouble(halfWidth, 0, w, h);
        default:         return wxRect2DDouble(0, 0, 0, 0); // Should not happen
    }
}

void ChartCornerInteractor::SetSelectedCorner(Corner corner)
{
    m_selectedCorner = corner;
}

ChartCornerInteractor::Corner ChartCornerInteractor::GetSelectedCorner() const
{
    return m_selectedCorner;
}

void ChartCornerInteractor::MoveSelectedCorner(int dx, int dy)
{
    if (m_selectedCorner == Corner::None) return;

    wxPoint2DDouble& corner_point = m_corners[static_cast<int>(m_selectedCorner)];
    wxRect2DDouble quadrant = GetQuadrant(m_selectedCorner);

    // Calculate new position and clamp within quadrant and image boundaries
    double newX = std::max(0.0, std::min(static_cast<double>(m_imageSize.GetWidth() - 1), std::max(quadrant.m_x, std::min(corner_point.m_x + dx, quadrant.m_x + quadrant.m_width))));
    double newY = std::max(0.0, std::min(static_cast<double>(m_imageSize.GetHeight() - 1), std::max(quadrant.m_y, std::min(corner_point.m_y + dy, quadrant.m_y + quadrant.m_height))));

    corner_point.m_x = newX;
    corner_point.m_y = newY;
}

void ChartCornerInteractor::SetCornerPosition(Corner corner, wxPoint2DDouble newPos) {
    if (corner == Corner::None || static_cast<int>(corner) >= m_corners.size()) {
        return;
    }

    wxRect2DDouble quadrant = GetQuadrant(corner);

    // Clamp the new position within the allowed quadrant and image boundaries
    double newX = std::max(0.0, std::min(static_cast<double>(m_imageSize.GetWidth() - 1), std::max(quadrant.m_x, std::min(newPos.m_x, quadrant.m_x + quadrant.m_width))));
    double newY = std::max(0.0, std::min(static_cast<double>(m_imageSize.GetHeight() - 1), std::max(quadrant.m_y, std::min(newPos.m_y, quadrant.m_y + quadrant.m_height))));

    m_corners[static_cast<int>(corner)] = wxPoint2DDouble(newX, newY);
}