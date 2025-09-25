// File: gui/ResultsController.hpp
/**
 * @file gui/ResultsController.hpp
 * @brief Declares a controller class for the ResultsPanel's logic.
 */
#pragma once
#include <memory>
#include <string>
#include <wx/event.h>

// Forward declarations
class DynaRangeFrame;
class ImageViewer;
class ResultsGridManager;
class wxSplitterEvent;

class ResultsController {
public:
    explicit ResultsController(DynaRangeFrame* frame);
    ~ResultsController();

    // Methods to update the view
    void LoadGraphImage(const std::string& path);
    void LoadLogoImage();
    bool DisplayResults(const std::string& csv_path);
    void SetUiState(bool is_processing);
    void OnRightPanelSize(wxSizeEvent& event);    
    // Event handling logic
    void OnSplitterSashDClick(wxSplitterEvent& event);
    void OnSize();

private:
    DynaRangeFrame* m_frame;
    std::unique_ptr<ImageViewer> m_imageViewer;
    std::unique_ptr<ResultsGridManager> m_gridManager;
    int m_lastSashPosition;
};