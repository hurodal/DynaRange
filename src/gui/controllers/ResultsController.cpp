// File: gui/ResultsController.cpp
/**
 * @file gui/ResultsController.cpp
 * @brief Implements the ResultsController class.
 */
#include "ResultsController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../helpers/ImageViewer.hpp"
#include "../helpers/ResultsGridManager.hpp"

ResultsController::ResultsController(DynaRangeFrame* frame) : m_frame(frame), m_lastSashPosition(350)
{
    m_imageViewer = std::make_unique<ImageViewer>(m_frame->m_imageGraph);
    m_gridManager = std::make_unique<ResultsGridManager>(m_frame->m_cvsGrid);
    m_frame->m_splitterResults->SetSashPosition(m_lastSashPosition);
    // Esto asegura que HandleResize() se llame cuando el panel cambie de tamaño.
    m_frame->m_rightPanel->Bind(wxEVT_SIZE, &ResultsController::OnRightPanelSize, this);
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

void ResultsController::OnRightPanelSize(wxSizeEvent& event) {
    // Programar la actualización para el siguiente ciclo de eventos
    // Esto garantiza que el layout del panel haya terminado de calcularse.
    // Y la gráfica se repinte en su nuevo tamaño.
    m_frame->CallAfter([this]() {
        if (m_imageViewer) {
            m_imageViewer->HandleResize();
        }
    });
    // Propaga el evento para que el layout funcione correctamente.
    event.Skip();
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

        // Forzar layout y refresco en el panel CORRECTO
        // El gauge y el bitmap están en m_rightPanel, no en m_resultsPanel.
        m_frame->m_rightPanel->Layout();
        m_frame->m_rightPanel->Refresh();
    } else {
        m_frame->m_processingGauge->Hide();
        m_frame->m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        m_frame->m_csvOutputStaticText->Show();
        m_frame->m_cvsGrid->Show();

        // Para el estado "no procesando", hacer lo mismo.
        m_frame->m_rightPanel->Layout();
        m_frame->m_rightPanel->Refresh();
    }
}

void ResultsController::OnSplitterSashDClick(wxSplitterEvent& event) {
    if (m_frame->m_splitterResults->IsSplit()) {
        m_lastSashPosition = event.GetSashPosition();
        m_frame->m_splitterResults->Unsplit(m_frame->m_leftPanel);
    } else {
        m_frame->m_splitterResults->SplitVertically(m_frame->m_leftPanel, m_frame->m_rightPanel, m_lastSashPosition);
    }
}

void ResultsController::OnSize() {
    m_imageViewer->HandleResize();
}