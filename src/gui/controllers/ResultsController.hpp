// File: src/gui/controllers/ResultsController.hpp
/**
 * @file gui/controllers/ResultsController.hpp
 * @brief Declares a controller class for the ResultsPanel's logic.
 */
#pragma once
#include <memory>
#include <string>
#include <wx/event.h>
#include <wx/image.h>

// Forward declarations
class DynaRangeFrame;
class ResultsGridManager;
class wxSplitterEvent;
class wxGridEvent;
class wxPaintEvent;

class ResultsController {
public:
    explicit ResultsController(DynaRangeFrame* frame);
    ~ResultsController();

    // Methods to update the view
    void DisplayImage(const wxImage& image);
    void LoadDefaultContent();
    bool DisplayResults(const std::string& csv_path);
    void SetUiState(bool is_processing);

    // Getter for the view
    const wxImage& GetSourceImage() const;

    // --- Event Handling Logic ---
    void OnSplitterSashDClick(wxSplitterEvent& event);
    void OnGridCellClick(wxGridEvent& event);
    void OnResultsCanvasPaint(wxPaintEvent& event); // New event handler

private:
    DynaRangeFrame* m_frame;
    std::unique_ptr<ResultsGridManager> m_gridManager;
    wxImage m_sourceImage; // In-memory source image for the canvas
    int m_lastSashPosition;
};