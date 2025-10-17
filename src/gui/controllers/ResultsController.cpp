// File: src/gui/controllers/ResultsController.cpp
/**
 * @file gui/controllers/ResultsController.cpp
 * @brief Implements the ResultsController class.
 */
#include "ResultsController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../helpers/ResultsGridManager.hpp"
#include "../utils/PathManager.hpp"
#include <wx/log.h>
#include <wx/image.h>
#include <wx/dcbuffer.h>   // For wxAutoBufferedPaintDC
#include <wx/graphics.h> // For wxGraphicsContext
#include <filesystem>
#include <string>
#include <cairo/cairo.h>

namespace fs = std::filesystem;

ResultsController::ResultsController(DynaRangeFrame* frame)
    : m_frame(frame), m_lastSashPosition(350)
{
    m_gridManager = std::make_unique<ResultsGridManager>(m_frame->m_cvsGrid);
    m_frame->m_splitterResults->SetSashPosition(m_lastSashPosition);
}

ResultsController::~ResultsController() = default;

void ResultsController::DisplayImage(const wxImage& image) {
    if (image.IsOk()) {
        m_sourceImage = image.Copy(); // Store a copy of the image
    } else {
        // If the incoming image is invalid, load the default logo.
        LoadDefaultContent();
    }
    m_frame->m_resultsCanvasPanel->Refresh();
}

void ResultsController::LoadDefaultContent() {
    // Load the logo as the default content into the source image member.
    ProgramOptions opts;
    PathManager path_manager(opts);
    fs::path logo_path = path_manager.GetAssetPath("assets/images/logo.png");
    
    if (fs::exists(logo_path)) {
        wxImage image(logo_path.string());
        if(image.IsOk()) {
            m_sourceImage = image;
        } else {
            m_sourceImage = wxImage(); // Invalidate on failure
        }
    } else {
        m_sourceImage = wxImage(); // Invalidate if logo not found
    }
    m_frame->m_resultsCanvasPanel->Refresh();
}

const wxImage& ResultsController::GetSourceImage() const
{
    return m_sourceImage;
}

bool ResultsController::DisplayResults(const std::string& csv_path) {
    bool success = m_gridManager->LoadFromCsv(csv_path);
    if (success) {
        // Force the sizer of the parent panel to recalculate the layout.
        // This makes the scrollbars appear immediately if the grid is too large.
        m_frame->m_leftPanel->Layout();
    }
    return success;
}

void ResultsController::SetUiState(bool is_processing) {
    if (is_processing) {
        m_frame->m_csvOutputStaticText->Hide();
        m_frame->m_cvsGrid->Hide();
        m_frame->m_generateGraphStaticText->SetLabel(_("Starting analysis... Please wait."));
        LoadDefaultContent(); // Show logo while processing
        m_frame->m_processingGauge->Show();
    } else {
        m_frame->m_processingGauge->Hide();
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        m_frame->m_csvOutputStaticText->Show();
        m_frame->m_cvsGrid->Show();
    }
    m_frame->m_rightPanel->Layout();
    m_frame->m_rightPanel->Refresh();
}

void ResultsController::OnGridCellClick(wxGridEvent& event) {
    // The specific row/cell clicked is no longer relevant for this action.
    // We simply notify the presenter that a click occurred in the grid area.
    m_frame->m_presenter->HandleGridCellClick();
    
    event.Skip();
}
void ResultsController::OnSplitterSashDClick(wxSplitterEvent& event) {
    if (m_frame->m_splitterResults->IsSplit()) {
        m_lastSashPosition = event.GetSashPosition();
        m_frame->m_splitterResults->Unsplit(m_frame->m_leftPanel);
    } else {
        m_frame->m_splitterResults->SplitVertically(m_frame->m_leftPanel, m_frame->m_rightPanel, m_lastSashPosition);
    }
}

void ResultsController::OnResultsCanvasPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(m_frame->m_resultsCanvasPanel);
    dc.Clear(); // Clear background

    const wxImage& sourceImage = GetSourceImage();
    if (sourceImage.IsOk()) {
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            double img_w = sourceImage.GetWidth();
            double img_h = sourceImage.GetHeight();
            const wxSize panel_size = dc.GetSize();
            
            // Calculate aspect-ratio correct scaling
            double scale_factor = std::min((double)panel_size.GetWidth() / img_w, (double)panel_size.GetHeight() / img_h);
            double final_width = img_w * scale_factor;
            double final_height = img_h * scale_factor;
            double offset_x = (panel_size.GetWidth() - final_width) / 2.0;
            double offset_y = (panel_size.GetHeight() - final_height) / 2.0;
            
            // Create a temporary, scaled bitmap for drawing in this paint event.
            // This is more efficient for resizing than scaling a bitmap.
            wxImage displayImage = sourceImage.Copy();
            displayImage.Rescale(final_width, final_height, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmapToDraw(displayImage);
            gc->DrawBitmap(bitmapToDraw, offset_x, offset_y, final_width, final_height);
            delete gc;
        }
    }
}