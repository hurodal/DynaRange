// File: src/gui/controllers/ChartController.hpp
/**
 * @file gui/controllers/ChartController.hpp
 * @brief Declares the controller for the Chart generation tab.
 */
#pragma once
#include <wx/event.h>
#include <wx/image.h>

// Forward declarations
class DynaRangeFrame;
struct ChartGeneratorOptions;
class wxSizeEvent;

class ChartController {
public:
    explicit ChartController(DynaRangeFrame* frame);
    ~ChartController();

    // Event handling logic
    void OnPreviewClick(wxCommandEvent& event);
    void OnCreateClick(wxCommandEvent& event);
    void OnColorSliderChanged(wxCommandEvent& event);
    void OnInputChanged(wxCommandEvent& event);

private:
    /**
     * @brief Reads all chart-related controls from the UI and creates an options struct.
     * @return A ChartGeneratorOptions struct populated with the current UI values.
     */
    ChartGeneratorOptions GetCurrentOptionsFromUi() const;

    /**
     * @brief Handles the size event for the right panel to refresh the image layout.
     * @param event The size event.
     */
    void OnRightPanelSize(wxSizeEvent& event);

    /**
     * @brief The core logic for scaling and setting the bitmap on the preview control.
     */
    void UpdateBitmapDisplay();

    DynaRangeFrame* m_frame; // Pointer to the parent frame
    wxImage m_chart_preview_image; // Stores the original preview image for resizing
};