// File: gui/controllers/ChartController.hpp
/**
 * @file gui/controllers/ChartController.hpp
 * @brief Declares the controller for the Chart generation tab.
 */
#pragma once
#include <wx/event.h>
#include <memory>

// Forward declarations
class DynaRangeFrame;
struct ChartGeneratorOptions;
class ImageViewer;

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
    
    DynaRangeFrame* m_frame; // Pointer to the parent frame
    
    // This controller will have its own dedicated image viewer for the thumbnail.
    std::unique_ptr<ImageViewer> m_thumbnailViewer;
};