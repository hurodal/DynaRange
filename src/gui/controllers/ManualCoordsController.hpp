// File: src/gui/controllers/ManualCoordsController.hpp
/**
 * @file ManualCoordsController.hpp
 * @brief Declares the controller for the Manual Coords tab.
 */
#pragma once
#include <wx/event.h>
#include <wx/image.h>
#include <vector>
#include <string>

// Forward declarations
class DynaRangeFrame;
class wxPaintEvent;
class wxSizeEvent;
class wxCommandEvent;
class wxFileDirPickerEvent;

class ManualCoordsController {
public:
    explicit ManualCoordsController(DynaRangeFrame* frame);
    ~ManualCoordsController();

    // --- Data Accessors ---
    std::vector<double> GetChartCoords() const;

    // --- Event Handling Logic ---
    void OnRawFileChanged(wxFileDirPickerEvent& event);
    void OnClearAllClick(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    
    /**
     * @brief Loads and displays the processed image from the main source RAW file.
     * @details This is called by the presenter when switching to this tab.
     */
    void LoadSourceImage();

private:
    /**
     * @brief Helper function to load and display a RAW file from a given path.
     * @param path The full path to the RAW file.
     */
    void DisplayRawFile(const std::string& path);

    DynaRangeFrame* m_frame;
    wxImage m_rawPreviewImage;
    int m_originalRawWidth = 0;
    int m_originalRawHeight = 0;
};