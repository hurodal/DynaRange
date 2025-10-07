// File: src/gui/controllers/ResultsController.hpp
/**
 * @file gui/controllers/ResultsController.hpp
 * @brief Declares a controller class for the ResultsPanel's logic.
 */
#pragma once
#include <memory>
#include <string>
#include <wx/event.h>

// Forward declarations
class DynaRangeFrame;
class ResultsGridManager;
class wxSplitterEvent;
class wxWebView; 

class ResultsController {
public:
    explicit ResultsController(DynaRangeFrame* frame);
    ~ResultsController();

    // Methods to update the view
    void LoadGraphImage(const std::string& path);
    void LoadDefaultContent(); 
    void LoadLogoImage();
    bool DisplayResults(const std::string& csv_path);
    void SetUiState(bool is_processing);

    // Event handling logic
    void OnSplitterSashDClick(wxSplitterEvent& event);

private:
    DynaRangeFrame* m_frame;
    wxWebView* m_webView; 
    std::unique_ptr<ResultsGridManager> m_gridManager;
    int m_lastSashPosition;
};
