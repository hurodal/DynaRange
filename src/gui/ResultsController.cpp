// File: gui/ResultsController.cpp
/**
 * @file gui/ResultsController.cpp
 * @brief Implements the ResultsController class.
 */
#include "ResultsController.hpp"
#include "DynaRangeFrame.hpp"
#include "ImageViewer.hpp"
#include "ResultsGridManager.hpp"

ResultsController::ResultsController(DynaRangeFrame* frame) : m_frame(frame), m_lastSashPosition(350)
{
    m_imageViewer = std::make_unique<ImageViewer>(m_frame->m_imageGraph);
    m_gridManager = std::make_unique<ResultsGridManager>(m_frame->m_cvsGrid);
    m_frame->m_splitter->SetSashPosition(m_lastSashPosition);
}

ResultsController::~ResultsController() = default;

void ResultsController::LoadGraphImage(const std::string& path) {
    wxString label = m_imageViewer->ShowGraph(path);
    m_frame->m_generateGraphStaticText->SetLabel(label);
}

void ResultsController::LoadLogoImage() {
    wxString label = m_imageViewer->ShowLogo();
    m_frame->m_generateGraphStaticText->SetLabel(label);
}

bool ResultsController::DisplayResults(const std::string& csv_path) {
    return m_gridManager->LoadFromCsv(csv_path);
}

void ResultsController::SetUiState(bool is_processing) {
    if (is_processing) {
        m_frame->m_csvOutputStaticText->Hide();
        m_frame->m_cvsGrid->Hide();
        m_frame->m_generateGraphStaticText->SetLabel(_("Processing... Please wait."));
        LoadLogoImage();
        m_frame->m_processingGauge->Show();

        // --- FIX: Forzar layout COMPLETO de la ventana ---
        // Esto garantiza que todos los controles, incluida la barra de progreso,
        // reciban su espacio correcto.
        m_frame->Layout();
        // Opcional: Forzar un refresco visual inmediato.
        m_frame->Refresh();
    } else {
        m_frame->m_processingGauge->Hide();
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        m_frame->m_csvOutputStaticText->Show();
        m_frame->m_cvsGrid->Show();

        // Para el estado "no procesando", forzar layout inmediatamente es suficiente.
        m_frame->Layout();
    }
}

void ResultsController::OnSplitterSashDClick(wxSplitterEvent& event) {
    if (m_frame->m_splitter->IsSplit()) {
        m_lastSashPosition = event.GetSashPosition();
        m_frame->m_splitter->Unsplit(m_frame->m_leftPanel);
    } else {
        m_frame->m_splitter->SplitVertically(m_frame->m_leftPanel, m_frame->m_rightPanel, m_lastSashPosition);
    }
}

void ResultsController::OnSize() {
    m_imageViewer->HandleResize();
}