// File: src/gui/controllers/ChartController.hpp
/**
 * @file gui/controllers/ChartController.hpp
 * @brief Declares the controller for the Chart generation tab.
 */
#pragma once
#include <wx/event.h>

// Forward declarations
class DynaRangeFrame;
struct ChartGeneratorOptions;
class wxSizeEvent;
class wxPaintEvent;
class wxCommandEvent;

class ChartController {

public: 
    explicit ChartController(DynaRangeFrame* frame);
    ~ChartController();

    // --- Event Handling Logic ---
    void OnCreateClick(wxCommandEvent& event);
    void OnColorSliderChanged(wxCommandEvent& event);
    void OnChartParamTextChanged(wxCommandEvent& event);
    void OnChartChartPatchChanged(wxCommandEvent& event);
    void OnChartPreviewPaint(wxPaintEvent& event);

    /**
     * @brief Reads current UI settings, generates chart thumbnail, and refreshes preview panel.
     */
    void UpdatePreview();

private:
    /**
     * @brief Gets the current chart generation options from the UI controls.
     * @return A struct containing the validated chart parameters.
     */
    ChartGeneratorOptions GetCurrentOptionsFromUi() const;

    DynaRangeFrame* m_frame; // Pointer to the parent frame
};