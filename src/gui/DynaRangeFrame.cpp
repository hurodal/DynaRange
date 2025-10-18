// File: src/gui/DynaRangeFrame.cpp
/**
 * @file DynaRangeFrame.cpp
 * @brief Implementation of the DynaRange GUI's main frame (the View).
 */
#include "DynaRangeFrame.hpp"
#include "Constants.hpp" 
#include "controllers/InputController.hpp"
#include "controllers/LogController.hpp"
#include "controllers/ResultsController.hpp"
#include "controllers/ChartController.hpp"
#include "../graphics/Constants.hpp"
#include "../core/utils/PathManager.hpp"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <fstream> 

// --- EVENT DEFINITIONS ---
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_PREVIEW_UPDATE_COMPLETE, wxCommandEvent);

// =============================================================================
// CONSTRUCTOR & DESTRUCTOR
// =============================================================================
DynaRangeFrame::DynaRangeFrame(wxWindow* parent)
    : MyFrameBase(parent), m_currentPreviewFile("")
{
    // --- Create Controllers for each tab ---
    m_inputController = std::make_unique<InputController>(this);
    m_logController = std::make_unique<LogController>(m_logOutputTextCtrl);
    m_resultsController = std::make_unique<ResultsController>(this);
    m_chartController = std::make_unique<ChartController>(this);

    // --- Set background styles for custom-drawn panels ---
    m_rawImagePreviewPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_resultsCanvasPanel = new wxPanel(m_webViewPlaceholderPanel, wxID_ANY);
    m_resultsCanvasPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    wxBoxSizer* placeholderSizer = new wxBoxSizer(wxVERTICAL);
    placeholderSizer->Add(m_resultsCanvasPanel, 1, wxEXPAND, 0);
    m_webViewPlaceholderPanel->SetSizer(placeholderSizer);
    m_chartPreviewPanel = new wxPanel(m_webView2PlaceholderPanel, wxID_ANY);
    m_chartPreviewPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // The new loupe panel must also be configured for custom painting.
    m_loupePanel->SetBackgroundStyle(wxBG_STYLE_PAINT);

    wxBoxSizer* chartPlaceholderSizer = new wxBoxSizer(wxVERTICAL);
    chartPlaceholderSizer->Add(m_chartPreviewPanel, 1, wxEXPAND, 0);
    m_webView2PlaceholderPanel->SetSizer(chartPlaceholderSizer);

    // --- Create the Presenter ---
    m_presenter = std::make_unique<GuiPresenter>(this);

    // --- Bind top-level and inter-controller events ---
    m_resultsCanvasPanel->Bind(wxEVT_PAINT, &ResultsController::OnResultsCanvasPaint, m_resultsController.get());
    m_chartPreviewPanel->Bind(wxEVT_PAINT, &ChartController::OnChartPreviewPaint, m_chartController.get());
    m_resultsCanvasPanel->Bind(wxEVT_SIZE, [this](wxSizeEvent& event){ this->m_resultsCanvasPanel->Refresh(); event.Skip(); });
    m_chartPreviewPanel->Bind(wxEVT_SIZE, [this](wxSizeEvent& event){ this->m_chartPreviewPanel->Refresh(); event.Skip(); });

    // Top-level events that remain handled by the Frame
    m_executeButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_removeAllFiles->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnRemoveAllFilesClick, this);
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    Bind(wxEVT_CLOSE_WINDOW, &DynaRangeFrame::OnClose, this);
    Bind(wxEVT_SIZE, &DynaRangeFrame::OnSize, this);
    m_mainNotebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &DynaRangeFrame::OnNotebookPageChanged, this);
    m_splitterResults->Bind(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED, &DynaRangeFrame::OnSplitterSashChanged, this);

    // --- Event bindings delegated directly to controllers ---
    m_clearAllCoordinates->Bind(wxEVT_BUTTON, &InputController::OnClearAllCoordsClick, m_inputController.get());
    m_addRawFilesButton->Bind(wxEVT_BUTTON, &InputController::OnAddFilesClick, m_inputController.get());
    m_saveLog->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get());
    m_darkFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &InputController::OnCalibrationFileChanged, m_inputController.get());
    m_saturationFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &InputController::OnCalibrationFileChanged, m_inputController.get());
    m_clearDarkFileButton->Bind(wxEVT_BUTTON, &InputController::OnClearDarkFile, m_inputController.get());
    m_clearSaturationFileButton->Bind(wxEVT_BUTTON, &InputController::OnClearSaturationFile, m_inputController.get());
    m_darkValueTextCtrl->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get());
    m_saturationValueTextCtrl->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get());
    m_removeRawFilesButton->Bind(wxEVT_BUTTON, &InputController::OnRemoveFilesClick, m_inputController.get());
    m_rawFileslistBox->Bind(wxEVT_LISTBOX, &InputController::OnListBoxSelectionChanged, m_inputController.get());
    m_rawFileslistBox->Bind(wxEVT_KEY_DOWN, &InputController::OnListBoxKeyDown, m_inputController.get());
    m_PlotChoice->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get());
    m_plotingChoice->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get());
    m_plotFormatChoice->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get());
    m_outputTextCtrl->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get());
    m_patchRatioSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &InputController::OnPatchRatioSliderChanged, m_inputController.get());
    m_patchRatioSlider->Bind(wxEVT_SCROLL_CHANGED, &InputController::OnPatchRatioSliderChanged, m_inputController.get());
    m_snrThresholdsValues->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get());
    m_drNormalizationSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &InputController::OnDrNormSliderChanged, m_inputController.get());
    m_drNormalizationSlider->Bind(wxEVT_SCROLL_CHANGED, &InputController::OnDrNormSliderChanged, m_inputController.get());
    m_chartPatchRowValue1->Bind(wxEVT_TEXT, &InputController::OnInputChartPatchChanged, m_inputController.get());
    m_chartPatchColValue1->Bind(wxEVT_TEXT, &InputController::OnInputChartPatchChanged, m_inputController.get());
    m_debugPatchesCheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnDebugPatchesCheckBoxChanged, m_inputController.get());
    m_debugPatchesFileNameValue->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get());
    R_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get());
    G1_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get());
    G2_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get());
    B_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get());
    AVG_ChoiceValue->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get());
    m_rParamSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &ChartController::OnColorSliderChanged, m_chartController.get());
    m_rParamSlider->Bind(wxEVT_SCROLL_CHANGED, &ChartController::OnColorSliderChanged, m_chartController.get());
    m_gParamSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &ChartController::OnColorSliderChanged, m_chartController.get());
    m_gParamSlider->Bind(wxEVT_SCROLL_CHANGED, &ChartController::OnColorSliderChanged, m_chartController.get());
    m_bParamSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &ChartController::OnColorSliderChanged, m_chartController.get());
    m_bParamSlider->Bind(wxEVT_SCROLL_CHANGED, &ChartController::OnColorSliderChanged, m_chartController.get());
    chartButtonPreview->Bind(wxEVT_BUTTON, &ChartController::OnPreviewClick, m_chartController.get());
    chartButtonCreate->Bind(wxEVT_BUTTON, &ChartController::OnCreateClick, m_chartController.get());
    m_InvGammaValue->Bind(wxEVT_TEXT, &ChartController::OnInputChanged, m_chartController.get());
    m_chartDimXValue->Bind(wxEVT_TEXT, &ChartController::OnInputChanged, m_chartController.get());
    m_chartDimWValue->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get());
    m_chartDimHValue->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get());
    m_chartPatchRowValue->Bind(wxEVT_TEXT, &ChartController::OnChartChartPatchChanged, m_chartController.get());
    m_chartPatchColValue->Bind(wxEVT_TEXT, &ChartController::OnChartChartPatchChanged, m_chartController.get());
    m_cvsGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &ResultsController::OnGridCellClick, m_resultsController.get());
    m_splitterResults->Bind(wxEVT_COMMAND_SPLITTER_DOUBLECLICKED, &ResultsController::OnSplitterSashDClick, m_resultsController.get());

    m_gaugeTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &DynaRangeFrame::OnGaugeTimer, this, m_gaugeTimer->GetId());
    m_dropTarget = new FileDropTarget(this);
    SetDropTarget(m_dropTarget);

    // --- Initial State Setup ---
    m_processingGauge->Hide();
    m_cvsGrid->Hide();
    m_csvOutputStaticText->Hide();
    m_generateGraphStaticText->SetLabel(_("Results will be shown here."));
    m_resultsController->LoadDefaultContent();

    m_presenter->UpdateCommandPreview();

    this->Layout();
}

DynaRangeFrame::~DynaRangeFrame() {
    delete m_gaugeTimer;
}

// =============================================================================
// PUBLIC INTERFACE FOR PRESENTER
// =============================================================================

// --- View Update Methods ---

void DynaRangeFrame::UpdateInputFileList(const std::vector<std::string>& files, int selected_index)
{
    m_inputController->UpdateInputFileList(files, selected_index);
}

void DynaRangeFrame::UpdateCommandPreview(const std::string& command) { 
    m_inputController->UpdateCommandPreview(command);
}

void DynaRangeFrame::DisplayResults(const std::string& csv_path) {
    if (!m_resultsController->DisplayResults(csv_path)) {
        ShowError(_("Error"), _("Could not open the results file: ") + csv_path);
    }
    m_mainNotebook->SetSelection(2);
}

void DynaRangeFrame::ShowError(const wxString& title, const wxString& message) { 
    wxMessageBox(message, title, wxOK | wxICON_ERROR, this);
}

void DynaRangeFrame::SetUiState(bool is_processing, int num_threads) {
    if (is_processing) {
        m_executeButton->SetLabel(_("Stop Processing"));
        m_executeButton->Enable(true);

        // Se cambia la selección de la pestaña usando el puntero del panel en lugar de un índice fijo.
        int page_index = m_mainNotebook->FindPage(m_logPanel);
        if (page_index != wxNOT_FOUND) {
            m_mainNotebook->SetSelection(page_index);
        }

        m_logController->Clear();
        m_gaugeTimer->Start(100);

        // Set the new parallel processing message.
        wxString status_label;
        if (num_threads > 1) {
            status_label = wxString::Format(_("Processing RAW files in parallel (%d processes)..."), num_threads);
        } else {
            status_label = _("Processing RAW files...");
        }
        m_generateGraphStaticText->SetLabel(status_label);
        m_processingGauge->Show();
    } else {
        m_executeButton->SetLabel(_("Execute"));
        m_executeButton->Enable(true);
        m_gaugeTimer->Stop();
        m_processingGauge->Hide();
        m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
    }
    
    m_inputPanel->Layout();
    m_resultsController->SetUiState(is_processing);
}

void DynaRangeFrame::OnExecuteClick(wxCommandEvent& event) {
    // Delegate all logic to the Presenter.
    m_presenter->OnExecuteButtonClicked();
}

void DynaRangeFrame::PostLogUpdate(const std::string& text) {
    wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_WORKER_UPDATE);
    event->SetString(text);
    wxQueueEvent(this, event);
}

void DynaRangeFrame::PostAnalysisComplete() { 
    wxQueueEvent(this, new wxCommandEvent(wxEVT_COMMAND_WORKER_COMPLETED)); 
}

void DynaRangeFrame::DisplayImage(const wxImage& image) { 
    m_resultsController->DisplayImage(image);
}

// --- Data Accessor Methods (Getters) ---

std::string DynaRangeFrame::GetDarkFilePath() const { return m_inputController->GetDarkFilePath(); }
std::string DynaRangeFrame::GetSaturationFilePath() const { return m_inputController->GetSaturationFilePath();
}
double DynaRangeFrame::GetDarkValue() const { return m_inputController->GetDarkValue(); }
double DynaRangeFrame::GetSaturationValue() const { return m_inputController->GetSaturationValue(); }
double DynaRangeFrame::GetPatchRatio() const { return m_inputController->GetPatchRatio();
}
std::string DynaRangeFrame::GetOutputFilePath() const { return m_inputController->GetOutputFilePath(); }
std::vector<double> DynaRangeFrame::GetSnrThresholds() const {  return m_inputController->GetSnrThresholds();  }
double DynaRangeFrame::GetDrNormalization() const { return m_inputController->GetDrNormalization();
}
int DynaRangeFrame::GetPolyOrder() const { return m_inputController->GetPolyOrder(); }
int DynaRangeFrame::GetPlotMode() const { return m_inputController->GetPlotMode(); }
int DynaRangeFrame::GetChartPatchesM() const { return m_inputController->GetChartPatchesM();
}
int DynaRangeFrame::GetChartPatchesN() const { return m_inputController->GetChartPatchesN();
}
std::string DynaRangeFrame::GetPrintPatchesFilename() const { return m_inputController->GetPrintPatchesFilename(); }
RawChannelSelection DynaRangeFrame::GetRawChannelSelection() const { return m_inputController->GetRawChannelSelection();
}
PlottingDetails DynaRangeFrame::GetPlottingDetails() const { return m_inputController->GetPlottingDetails(); }

// =============================================================================
// EVENT HANDLERS
// =============================================================================

void DynaRangeFrame::OnClose(wxCloseEvent& event) {
    if (m_presenter->IsWorkerRunning()) {
        m_presenter->RequestWorkerCancellation();
    }
    Destroy();
}

void DynaRangeFrame::OnGaugeTimer(wxTimerEvent& event) { m_processingGauge->Pulse();
}

void DynaRangeFrame::OnNotebookPageChanged(wxNotebookEvent& event) {
    // This event handler is currently not needed but is kept for future use.
    event.Skip();
}

std::vector<double> DynaRangeFrame::GetChartCoords() const { 
    return m_inputController->GetChartCoords(); 
}

void DynaRangeFrame::OnSize(wxSizeEvent& event) { event.Skip();}

void DynaRangeFrame::OnSplitterSashChanged(wxSplitterEvent& event) { event.Skip();
}

bool DynaRangeFrame::ShouldSaveLog() const {
    return m_inputController->ShouldSaveLog();
}

void DynaRangeFrame::OnRemoveAllFilesClick(wxCommandEvent& event) {
    m_presenter->RemoveAllInputFiles();
}

void DynaRangeFrame::OnWorkerCompleted(wxCommandEvent& event) {
    SetUiState(false); // Set UI to idle state (reenables controls, hides gauge)
    const ReportOutput& report = m_presenter->GetLastReport();

    // If the final_csv_path is empty, it means analysis was cancelled or failed.
    if (report.final_csv_path.empty()) {
        // Log already contains cancellation/error message. Just return.
        return;
    }

    // --- Analysis completed successfully ---
    const wxImage& summary_image = m_presenter->GetLastSummaryImage();

    // Display the results grid (this function no longer changes the tab)
    // We capture the success state to know if we should proceed.
    bool results_displayed = m_resultsController->DisplayResults(report.final_csv_path);

    if (results_displayed) {
        // If the grid was loaded, display the summary image if available
        if (summary_image.IsOk()) {
            DisplayImage(summary_image); // Updates image panel on Results tab
        } else if (GetPlotMode() != 0) {
            // If plot was requested but failed
            m_generateGraphStaticText->SetLabel(_("Results loaded, but summary plot failed."));
            m_resultsController->LoadDefaultContent(); // Show default logo
            m_logController->AppendText(_("\nError: Summary plot could not be generated."));
        } else {
            // If plot was not requested
            m_generateGraphStaticText->SetLabel(_("Results loaded. Plot generation was not requested."));
            m_resultsController->LoadDefaultContent(); // Show default logo
        }

        // --- Log Saving Logic ---
        if (ShouldSaveLog()) {
            ProgramOptions temp_opts;
            temp_opts.output_filename = GetOutputFilePath(); // Use CSV path as base
            PathManager paths(temp_opts);
            fs::path log_path = paths.GetCsvOutputPath().parent_path() / DynaRange::Gui::Constants::LOG_OUTPUT_FILENAME;

            wxString log_content = m_logOutputTextCtrl->GetValue();
            std::ofstream log_file(log_path);
            if (log_file.is_open()) {
                log_file << log_content.ToStdString();
                log_file.close();
                m_logController->AppendText(wxString::Format(_("\n[INFO] Log saved to: %s\n"), log_path.string()));
            } else {
                m_logController->AppendText(wxString::Format(_("\n[ERROR] Could not save log to file: %s\n"), log_path.string()));
            }
        }

        // --- INICIO DE LA MODIFICACIÓN ---
        // Force selection of the "Results" tab AFTER everything else is done.
        int page_index = m_mainNotebook->FindPage(m_resultsPanel);
        if (page_index != wxNOT_FOUND) {
            m_mainNotebook->SetSelection(page_index);
        }
        // --- FIN DE LA MODIFICACIÓN ---

    } else {
         // Handle the case where DisplayResults failed (e.g., couldn't read CSV)
         ShowError(_("Error"), _("Could not open or process the results file: ") + wxString(report.final_csv_path));
         // Stay on the Log tab or switch back to Input? For now, do nothing extra.
    }
}
void DynaRangeFrame::OnWorkerUpdate(wxThreadEvent& event) {
    wxString log_message = event.GetString();
    m_logController->AppendText(log_message);
}

DynaRange::Graphics::Constants::PlotOutputFormat DynaRangeFrame::GetPlotFormat() const {
    return m_inputController->GetPlotFormat();
}

bool DynaRangeFrame::ShouldGenerateIndividualPlots() const {
    return m_inputController->ShouldGenerateIndividualPlots();
}

bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) {
    if (m_owner && m_owner->m_inputController) {
        m_owner->m_inputController->AddDroppedFiles(filenames);
    }
    return true;
}

bool DynaRangeFrame::ValidateSnrThresholds() const {
    return m_inputController->ValidateSnrThresholds();
}

void DynaRangeFrame::UpdateRawPreview(const std::string& path) {
    if (m_inputController) {
        m_inputController->DisplayPreviewImage(path);
    }
}

void DynaRangeFrame::SetExecuteButtonToStoppingState()
{
    m_executeButton->SetLabel(_("Waiting stop..."));
    m_executeButton->Enable(false);
    m_inputPanel->Layout();
}

bool DynaRangeFrame::ShouldEstimateBlackLevel() const {
    return m_inputController->ShouldEstimateBlackLevel();
}

bool DynaRangeFrame::ShouldEstimateSaturationLevel() const {
    return m_inputController->ShouldEstimateSaturationLevel();
}