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

class ChartController {
public:
    explicit ChartController(DynaRangeFrame* frame);
    ~ChartController();

    // --- Event Handling Logic ---
    void OnPreviewClick(wxCommandEvent& event);
    void OnCreateClick(wxCommandEvent& event);
    void OnColorSliderChanged(wxCommandEvent& event);
    void OnInputChanged(wxCommandEvent& event);
    void OnChartChartPatchChanged(wxCommandEvent& event);

private:
    ChartGeneratorOptions GetCurrentOptionsFromUi() const;

    DynaRangeFrame* m_frame; // Pointer to the parent frame
};
