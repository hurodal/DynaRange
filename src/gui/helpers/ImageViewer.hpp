// File: gui/ImageViewer.hpp
/**
 * @file gui/ImageViewer.hpp
 * @brief Declares a helper class to manage image display and scaling.
 * @details This class encapsulates all logic related to loading an image,
 * storing it, and handling the responsive scaling to fit its container.
 * This adheres to the Single Responsibility Principle.
 */
#pragma once

#include <wx/image.h>
#include <wx/statbmp.h>
#include <wx/event.h>
#include <string>

class ImageViewer {
public:
    /**
     * @brief Constructor.
     * @param imageControl A pointer to the wxStaticBitmap control this manager will handle.
     */
    explicit ImageViewer(wxStaticBitmap* imageControl);

    /**
     * @brief Loads and displays a graph from a file path.
     * @param path The path to the image file.
     * @return A descriptive string for the label (e.g., "Generated Graph: ...").
     */
    wxString ShowGraph(const std::string& path);

    /**
     * @brief Loads and displays the application logo.
     * @return A descriptive string for the label (e.g., "Welcome...").
     */
    wxString ShowLogo();

    /**
     * @brief Handles a resize event from the parent frame.
     * @details This should be called from the frame's OnSize handler.
     */
    void HandleResize();

private:
    /**
     * @brief The core logic for scaling and setting the bitmap.
     */
    void UpdateBitmapDisplay();

    /// @brief Pointer to the UI control that displays the image.
    wxStaticBitmap* m_imageControl;

    /// @brief Stores the original, unscaled image to prevent quality loss.
    wxImage m_originalImage;
};