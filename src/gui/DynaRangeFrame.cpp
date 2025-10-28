// File: src/gui/DynaRangeFrame.cpp
/**
 * @file DynaRangeFrame.cpp
 * @brief Implementation of the DynaRange GUI's main frame (the View).
 */
#include "DynaRangeFrame.hpp"
#include "controllers/InputController.hpp" // Need full definition for getter return type
#include "controllers/LogController.hpp"
#include "controllers/ResultsController.hpp"
#include "controllers/ChartController.hpp"
#include "../artifacts/data/ReportWriter.hpp"
#include "../graphics/Constants.hpp"
#include "../core/utils/OutputNamingContext.hpp"     // Added include for log saving
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/dcclient.h>   // For event handlers if needed later
#include <wx/graphics.h>   // For event handlers if needed later
#include <wx/dcbuffer.h>   // For event handlers if needed later
#include <filesystem>      // For fs::path

// Use alias for filesystem
namespace fs = std::filesystem;

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
    // --- Create Controllers ---
    m_inputController = std::make_unique<InputController>(this);
    m_logController = std::make_unique<LogController>(m_logOutputTextCtrl);
    m_resultsController = std::make_unique<ResultsController>(this);
    m_chartController = std::make_unique<ChartController>(this);

    // --- Set background styles ---
    m_rawImagePreviewPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_resultsCanvasPanel = new wxPanel(m_webViewPlaceholderPanel, wxID_ANY);
    m_resultsCanvasPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    wxBoxSizer* placeholderSizer = new wxBoxSizer(wxVERTICAL);
    placeholderSizer->Add(m_resultsCanvasPanel, 1, wxEXPAND, 0);
    m_webViewPlaceholderPanel->SetSizer(placeholderSizer);
    m_chartPreviewPanel = new wxPanel(m_webView2PlaceholderPanel, wxID_ANY);
    m_chartPreviewPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_loupePanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    wxBoxSizer* chartPlaceholderSizer = new wxBoxSizer(wxVERTICAL);
    chartPlaceholderSizer->Add(m_chartPreviewPanel, 1, wxEXPAND, 0);
    m_webView2PlaceholderPanel->SetSizer(chartPlaceholderSizer);

    // --- Create Presenter ---
    m_presenter = std::make_unique<GuiPresenter>(this);

    // --- Bind top-level and inter-controller events ---
    m_resultsCanvasPanel->Bind(wxEVT_PAINT, &ResultsController::OnResultsCanvasPaint, m_resultsController.get());
    m_chartPreviewPanel->Bind(wxEVT_PAINT, &ChartController::OnChartPreviewPaint, m_chartController.get());
    m_resultsCanvasPanel->Bind(wxEVT_SIZE, [this](wxSizeEvent& event){ this->m_resultsCanvasPanel->Refresh(); event.Skip(); });
    m_chartPreviewPanel->Bind(wxEVT_SIZE, [this](wxSizeEvent& event){ this->m_chartPreviewPanel->Refresh(); event.Skip(); });

    // --- Top-level events handled by Frame ---
    m_executeButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_removeAllFiles->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnRemoveAllFilesClick, this);
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    Bind(wxEVT_CLOSE_WINDOW, &DynaRangeFrame::OnClose, this);
    Bind(wxEVT_SIZE, &DynaRangeFrame::OnSize, this);
    m_mainNotebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &DynaRangeFrame::OnNotebookPageChanged, this);
    m_splitterResults->Bind(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED, &DynaRangeFrame::OnSplitterSashChanged, this);

    // --- Event bindings delegated to controllers ---
    // InputController events
    m_clearAllCoordinates->Bind(wxEVT_BUTTON, &InputController::OnClearAllCoordsClick, m_inputController.get());
    m_addRawFilesButton->Bind(wxEVT_BUTTON, &InputController::OnAddFilesClick, m_inputController.get());
    m_saveLog->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_darkFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &InputController::OnCalibrationFileChanged, m_inputController.get());
    m_saturationFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &InputController::OnCalibrationFileChanged, m_inputController.get());
    m_clearDarkFileButton->Bind(wxEVT_BUTTON, &InputController::OnClearDarkFile, m_inputController.get());
    m_clearSaturationFileButton->Bind(wxEVT_BUTTON, &InputController::OnClearSaturationFile, m_inputController.get());
    m_darkValueTextCtrl->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_saturationValueTextCtrl->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_removeRawFilesButton->Bind(wxEVT_BUTTON, &InputController::OnRemoveFilesClick, m_inputController.get());
    m_rawFileslistBox->Bind(wxEVT_LISTBOX, &InputController::OnListBoxSelectionChanged, m_inputController.get());
    m_rawFileslistBox->Bind(wxEVT_KEY_DOWN, &InputController::OnListBoxKeyDown, m_inputController.get());
    m_PlotChoice->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_plotingChoice->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_plotFormatChoice->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_outputTextCtrl->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_patchRatioSlider->Bind(wxEVT_SCROLL_CHANGED, &InputController::OnPatchRatioSliderChanged, m_inputController.get());
    m_snrThresholdsValues->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_drNormalizationSlider->Bind(wxEVT_SCROLL_CHANGED, &InputController::OnDrNormSliderChanged, m_inputController.get());
    m_chartPatchRowValue1->Bind(wxEVT_TEXT, &InputController::OnInputChartPatchChanged, m_inputController.get());
    m_chartPatchColValue1->Bind(wxEVT_TEXT, &InputController::OnInputChartPatchChanged, m_inputController.get());
    m_debugPatchesCheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnDebugPatchesCheckBoxChanged, m_inputController.get());
    m_debugPatchesFileNameValue->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    R_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    G1_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    G2_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    B_checkBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    AVG_ChoiceValue->Bind(wxEVT_CHOICE, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    allIsosCheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_plotParamScattersCheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_plotParamCurveCheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_plotParamLabelsCheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnInputChanged, m_inputController.get()); // Generic OK
    m_fromExifOutputCheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnFromExifCheckBoxChanged, m_inputController.get()); // Specific handler
    m_subnameOutputcheckBox->Bind(wxEVT_CHECKBOX, &InputController::OnSubnameCheckBoxChanged, m_inputController.get()); // Specific handler
    m_subnameTextCtrl->Bind(wxEVT_TEXT, &InputController::OnInputChanged, m_inputController.get()); // Generic OK for preview

    // ChartController events
    m_rParamSlider->Bind(wxEVT_SCROLL_CHANGED, &ChartController::OnColorSliderChanged, m_chartController.get());
    m_gParamSlider->Bind(wxEVT_SCROLL_CHANGED, &ChartController::OnColorSliderChanged, m_chartController.get());
    m_bParamSlider->Bind(wxEVT_SCROLL_CHANGED, &ChartController::OnColorSliderChanged, m_chartController.get());
    chartButtonCreate->Bind(wxEVT_BUTTON, &ChartController::OnCreateClick, m_chartController.get());
    m_InvGammaValue->Bind(wxEVT_TEXT, &ChartController::OnChartParamTextChanged, m_chartController.get());
    m_chartDimXValue->Bind(wxEVT_TEXT, &ChartController::OnChartParamTextChanged, m_chartController.get());
    m_chartDimWValue->Bind(wxEVT_TEXT, &ChartController::OnChartParamTextChanged, m_chartController.get());
    m_chartDimHValue->Bind(wxEVT_TEXT, &ChartController::OnChartParamTextChanged, m_chartController.get());
    m_chartPatchRowValue->Bind(wxEVT_TEXT, &ChartController::OnChartChartPatchChanged, m_chartController.get());
    m_chartPatchColValue->Bind(wxEVT_TEXT, &ChartController::OnChartChartPatchChanged, m_chartController.get());

    // ResultsController events
    m_cvsGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &ResultsController::OnGridCellClick, m_resultsController.get());
    m_splitterResults->Bind(wxEVT_COMMAND_SPLITTER_DOUBLECLICKED, &ResultsController::OnSplitterSashDClick, m_resultsController.get());

    // Timer
    m_gaugeTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &DynaRangeFrame::OnGaugeTimer, this, m_gaugeTimer->GetId());

    // Drop Target
    m_dropTarget = new FileDropTarget(this);
    SetDropTarget(m_dropTarget);

    // --- Initial State Setup ---
    m_processingGauge->Hide();
    m_cvsGrid->Hide();
    m_csvOutputStaticText->Hide();
    m_generateGraphStaticText->SetLabel(_("Results will be shown here."));
    m_resultsController->LoadDefaultContent();
    m_presenter->UpdateCommandPreview();
    m_chartController->UpdatePreview();

    // Initial Layout
    this->Layout();
}

DynaRangeFrame::~DynaRangeFrame() {
    // Stop and delete the timer
    if (m_gaugeTimer) {
        m_gaugeTimer->Stop();
        delete m_gaugeTimer;
    }
    // Note: unique_ptrs for controllers handle their own cleanup.
    // Note: m_dropTarget needs consideration - wxWidgets might manage it? Check wx docs. If not, delete here.
}

// =============================================================================
// PUBLIC INTERFACE FOR PRESENTER (View Update Methods)
// =============================================================================

void DynaRangeFrame::UpdateInputFileList(const std::vector<std::string>& files, int selected_index)
{
    // Delegate to InputController to update the listbox
    if (m_inputController) {
        m_inputController->UpdateInputFileList(files, selected_index);
    }
}

void DynaRangeFrame::UpdateCommandPreview(const std::string& command) {
    // Delegate to InputController to update the command text control
    if (m_inputController) {
        m_inputController->UpdateCommandPreview(command);
    }
}

void DynaRangeFrame::DisplayResults(const std::string& csv_path) {
    // Delegate to ResultsController to load CSV into grid
    if (m_resultsController) {
        if (!m_resultsController->DisplayResults(csv_path)) {
            // Show error if loading failed
            ShowError(_("Error"), _("Could not open or display the results file: ") + csv_path);
        }
    }
    // Switch to the results tab (Index 3, might need adjustment if tabs change)
    // FindPage is safer
    int page_index = m_mainNotebook->FindPage(m_resultsPanel);
    if (page_index != wxNOT_FOUND) {
        m_mainNotebook->SetSelection(page_index);
    }
}

void DynaRangeFrame::ShowError(const wxString& title, const wxString& message) {
    // Display a standard error message box
    wxMessageBox(message, title, wxOK | wxICON_ERROR, this);
}

void DynaRangeFrame::SetUiState(bool is_processing, int num_threads) {
    if (is_processing) {
        // --- UI State: Processing Active ---
        m_executeButton->SetLabel(_("Stop Processing"));
        m_executeButton->Enable(true); // Enable stop button

        // Switch to the Log tab
        int page_index = m_mainNotebook->FindPage(m_logPanel);
        if (page_index != wxNOT_FOUND) {
            m_mainNotebook->SetSelection(page_index);
        }

        // Clear previous log and start gauge animation
        if(m_logController) m_logController->Clear();
        if(m_gaugeTimer) m_gaugeTimer->Start(100); // Pulse every 100ms

        // Update status label based on thread count
        wxString status_label;
        if (num_threads > 1) {
            status_label = wxString::Format(_("Processing RAW files in parallel (%d processes)..."), num_threads);
        } else {
            status_label = _("Processing RAW files...");
        }
        m_generateGraphStaticText->SetLabel(status_label); // Show status on results tab
        m_processingGauge->Show(); // Show gauge animation
    } else {
        // --- UI State: Idle ---
        m_executeButton->SetLabel(_("Execute"));
        m_executeButton->Enable(true); // Re-enable execute button
        if(m_gaugeTimer) m_gaugeTimer->Stop(); // Stop gauge animation
        m_processingGauge->Hide(); // Hide gauge
        m_generateGraphStaticText->SetLabel(_("Generated Graph:")); // Reset status label
    }

    // Force layout updates
    m_inputPanel->Layout(); // Ensure Input panel layout is correct
    if(m_resultsController) m_resultsController->SetUiState(is_processing); // Update Results panel state
}

void DynaRangeFrame::PostLogUpdate(const std::string& text) {
    // Create a thread event to safely pass the log string to the main thread
    wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_WORKER_UPDATE);
    event->SetString(wxString::FromUTF8(text)); // Convert std::string to wxString
    wxQueueEvent(this, event); // Post the event to the frame's event queue
}

void DynaRangeFrame::PostAnalysisComplete() {
    // Post a simple command event to signal completion
    wxQueueEvent(this, new wxCommandEvent(wxEVT_COMMAND_WORKER_COMPLETED));
}

void DynaRangeFrame::DisplayImage(const wxImage& image) {
    // Delegate image display to the ResultsController
    if(m_resultsController) {
        m_resultsController->DisplayImage(image);
    }
}

void DynaRangeFrame::UpdateRawPreview(const std::string& path) {
    // Delegate preview update to the InputController (which uses PreviewController)
    if (m_inputController) {
        m_inputController->DisplayPreviewImage(path);
    }
}

void DynaRangeFrame::SetExecuteButtonToStoppingState()
{
    // Update button label and disable while waiting for thread to stop
    m_executeButton->SetLabel(_("Waiting stop..."));
    m_executeButton->Enable(false);
    m_inputPanel->Layout(); // Update layout if needed
}


// =============================================================================
// DATA ACCESSOR METHODS (GETTERS) - Delegating to InputController
// =============================================================================

std::string DynaRangeFrame::GetDarkFilePath() const {
    return m_inputController ? m_inputController->GetDarkFilePath() : "";
}
std::string DynaRangeFrame::GetSaturationFilePath() const {
    return m_inputController ? m_inputController->GetSaturationFilePath() : "";
}
double DynaRangeFrame::GetDarkValue() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetDarkValue() : DEFAULT_BLACK_LEVEL;
}
double DynaRangeFrame::GetSaturationValue() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetSaturationValue() : DEFAULT_SATURATION_LEVEL;
}
double DynaRangeFrame::GetPatchRatio() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetPatchRatio() : DEFAULT_PATCH_RATIO;
}
std::string DynaRangeFrame::GetOutputFilePath() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetOutputFilePath() : DEFAULT_OUTPUT_FILENAME;
}
std::vector<double> DynaRangeFrame::GetSnrThresholds() const {
     // Use constant from ArgumentsOptions.hpp for fallback
     return m_inputController ? m_inputController->GetSnrThresholds() : DEFAULT_SNR_THRESHOLDS_DB;
}
double DynaRangeFrame::GetDrNormalization() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetDrNormalization() : DEFAULT_DR_NORMALIZATION_MPX;
}
int DynaRangeFrame::GetPolyOrder() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetPolyOrder() : DEFAULT_POLY_ORDER;
}
int DynaRangeFrame::GetPlotMode() const {
    // Default to "Graphic + Long Command" index if no controller
    return m_inputController ? m_inputController->GetPlotMode() : 3;
}
DynaRange::Graphics::Constants::PlotOutputFormat DynaRangeFrame::GetPlotFormat() const {
    // Default to PNG if no controller
    return m_inputController ? m_inputController->GetPlotFormat() : DynaRange::Graphics::Constants::PlotOutputFormat::PNG;
}
std::vector<double> DynaRangeFrame::GetChartCoords() const {
    // Return empty vector if no controller
    return m_inputController ? m_inputController->GetChartCoords() : std::vector<double>();
}
int DynaRangeFrame::GetChartPatchesM() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetChartPatchesM() : DEFAULT_CHART_PATCHES_M;
}
int DynaRangeFrame::GetChartPatchesN() const {
    // Use constant from ArgumentsOptions.hpp for fallback
    return m_inputController ? m_inputController->GetChartPatchesN() : DEFAULT_CHART_PATCHES_N;
}
std::string DynaRangeFrame::GetPrintPatchesFilename() const {
    // Use sentinel value as fallback
    return m_inputController ? m_inputController->GetPrintPatchesFilename() : "_USE_DEFAULT_PRINT_PATCHES_";
}
RawChannelSelection DynaRangeFrame::GetRawChannelSelection() const {
    // Default construct RawChannelSelection if no controller
    return m_inputController ? m_inputController->GetRawChannelSelection() : RawChannelSelection{};
}
PlottingDetails DynaRangeFrame::GetPlottingDetails() const {
    // Default construct PlottingDetails if no controller
    return m_inputController ? m_inputController->GetPlottingDetails() : PlottingDetails{};
}
bool DynaRangeFrame::ValidateSnrThresholds() const {
    // Assume valid if no controller (shouldn't happen in practice)
    return m_inputController ? m_inputController->ValidateSnrThresholds() : true;
}
bool DynaRangeFrame::ShouldSaveLog() const {
    // Default to false if no controller
    return m_inputController ? m_inputController->ShouldSaveLog() : false;
}
bool DynaRangeFrame::ShouldGenerateIndividualPlots() const {
    // Default to false if no controller
    return m_inputController ? m_inputController->ShouldGenerateIndividualPlots() : false;
}
bool DynaRangeFrame::ShouldEstimateBlackLevel() const {
    // Default to true (estimate) if no controller
    return m_inputController ? m_inputController->ShouldEstimateBlackLevel() : true;
}
bool DynaRangeFrame::ShouldEstimateSaturationLevel() const {
    // Default to true (estimate) if no controller
    return m_inputController ? m_inputController->ShouldEstimateSaturationLevel() : true;
}

// --- Public getter implementation for InputController ---
/**
 * @brief Provides access to the InputController instance.
 * @return A pointer to the InputController, or nullptr if not initialized.
 */
InputController* DynaRangeFrame::GetInputController() const {
    // unique_ptr::get() returns the managed raw pointer
    return m_inputController.get();
}


// =============================================================================
// EVENT HANDLERS (Frame Level)
// =============================================================================

void DynaRangeFrame::OnExecuteClick(wxCommandEvent& event) {
    // Delegate click logic entirely to the Presenter
    if(m_presenter) {
        m_presenter->OnExecuteButtonClicked();
    }
    // event.Skip(); // Typically not needed for button events
}

void DynaRangeFrame::OnClose(wxCloseEvent& event) {
    // If worker thread is running, request cancellation before closing
    if (m_presenter && m_presenter->IsWorkerRunning()) {
        m_presenter->RequestWorkerCancellation();
        // Optionally wait for thread to finish or just proceed with closing
    }
    Destroy(); // Close and destroy the frame
}

void DynaRangeFrame::OnGaugeTimer(wxTimerEvent& event) {
    // Pulse the gauge during processing for visual feedback
    if(m_processingGauge) {
        m_processingGauge->Pulse();
    }
}

void DynaRangeFrame::OnNotebookPageChanged(wxNotebookEvent& event) {
    // Can add logic here if specific actions need to happen on tab change
    // For example, refreshing a specific panel when it becomes visible
    // int selection = event.GetSelection();
    // if (selection == X) { /* Do something */ }
    event.Skip(); // Allow default tab change behavior
}


void DynaRangeFrame::OnSize(wxSizeEvent& event) {
    // Allow the default resize behavior and sizer recalculations
    event.Skip();
}

void DynaRangeFrame::OnSplitterSashChanged(wxSplitterEvent& event) {
    // Can add logic here if needed when splitter position changes
    event.Skip(); // Allow default sash repositioning
}


void DynaRangeFrame::OnRemoveAllFilesClick(wxCommandEvent& event) {
    // Delegate 'Remove All' action to the Presenter
    if(m_presenter) {
        m_presenter->RemoveAllInputFiles();
    }
    // event.Skip(); // Typically not needed
}

/**
 * @brief Handles the completion event from the background worker thread.
 * Updates the UI state, displays results (grid and plot), and potentially
 * saves the log file using ArtifactFactory.
 * @param event The command event details.
 */
void DynaRangeFrame::OnWorkerCompleted(wxCommandEvent& event) {
    SetUiState(false); // Set UI back to idle state
    const ReportOutput& report = m_presenter->GetLastReport(); // Get results from presenter

    // Check if analysis was cancelled or failed (indicated by empty CSV path)
    if (report.final_csv_path.empty()) {
        // Log message should already indicate cancellation or fatal error
        // Optionally update status bar or a label here
        m_generateGraphStaticText->SetLabel(_("Processing cancelled or failed. Check log."));
        return; // Stop further processing
    }

    // --- Analysis completed successfully ---
    const wxImage& summary_image = m_presenter->GetLastSummaryImage(); // Get generated summary image

    // Try displaying the results grid from the CSV file
    bool results_displayed = false;
    if (m_resultsController) {
         results_displayed = m_resultsController->DisplayResults(report.final_csv_path);
    }

    if (results_displayed) {
        // --- Grid Displayed Successfully ---

        // Display the summary plot image if it was generated successfully
        if (summary_image.IsOk()) {
            DisplayImage(summary_image); // Update image panel on Results tab
            m_generateGraphStaticText->SetLabel(_("Generated Graph:")); // Update status label
        }
        // Handle cases where plotting was requested but failed, or not requested
        else if (GetPlotMode() != 0) { // Plotting was enabled
            m_generateGraphStaticText->SetLabel(_("Results loaded, but summary plot failed."));
            if(m_resultsController) m_resultsController->LoadDefaultContent(); // Show default logo/placeholder
            if(m_logController) m_logController->AppendText(_("\nError: Summary plot could not be generated. Check log."));
        } else { // Plotting was disabled
            m_generateGraphStaticText->SetLabel(_("Results loaded. Plot generation was not requested."));
            if(m_resultsController) m_resultsController->LoadDefaultContent(); // Show default logo/placeholder
        }

        // --- Save Log File if requested ---
        if (ShouldSaveLog() && !report.final_csv_path.empty()) { // Also check if CSV path exists
            // Get log content
            std::string log_content_std = m_logOutputTextCtrl->GetValue().ToStdString();

            // Create context for Factory
            OutputNamingContext naming_ctx_log;
            // Get last run options used (Presenter should provide this)
            const ProgramOptions& lastOpts = m_presenter->GetLastRunOptions();
            if (!lastOpts.input_files.empty()){
                 // Get exif name from the last report if available
                 if (!report.curve_data.empty()) {
                     naming_ctx_log.camera_name_exif = report.curve_data[0].camera_model;
                 }
            }
             // Determine effective name based on last run's GUI flags
            if (lastOpts.gui_use_camera_suffix) {
                naming_ctx_log.effective_camera_name_for_output = lastOpts.gui_use_exif_camera_name ?
                                                                  naming_ctx_log.camera_name_exif :
                                                                  lastOpts.gui_manual_camera_name;
            } else {
                 naming_ctx_log.effective_camera_name_for_output = "";
            }

            // Determine base directory from CSV path
            fs::path output_dir = fs::path(report.final_csv_path).parent_path();

            // Get the log stream from the controller
            // std::ostream* factory_log_stream_ptr = m_logController ? m_logController->GetLogStream() : nullptr; // LÍNEA ELIMINADA
            // Use std::cerr as a fallback if the controller or its stream is unavailable
            // std::ostream& factory_log_stream = factory_log_stream_ptr ? *factory_log_stream_ptr : std::cerr; // LÍNEA ELIMINADA

            // Use ArtifactFactory to create and save the log file
            std::optional<fs::path> log_path_opt = ArtifactFactory::Report::CreateLogFile(
                log_content_std,
                naming_ctx_log,
                output_dir
                // factory_log_stream // PARÁMETRO ELIMINADO
            );
            
            // Log success or failure *after* the call, using the GUI's LogController
            if (log_path_opt) {
                if(m_logController) m_logController->AppendText(wxString::Format(_("\n[INFO] Log saved to: %s\n"), log_path_opt->string()));
            } else {
                if(m_logController) m_logController->AppendText(wxString::Format(_("\n[ERROR] Could not save log to file: %s\n"), (output_dir / "DynaRange Analysis Results.txt").string())); // Approximate path for error
            }
        }

        // --- Switch to Results Tab ---
        // Force selection of the "Results" tab after all updates are done
        int page_index = m_mainNotebook->FindPage(m_resultsPanel);
        if (page_index != wxNOT_FOUND) {
            m_mainNotebook->SetSelection(page_index);
        }

    } else {
         // --- Grid Display Failed ---
         // Show error message if DisplayResults failed (e.g., couldn't read CSV)
         ShowError(_("Error"), _("Could not open or process the results file: ") + wxString(report.final_csv_path));
         // Optional: Switch back to Log or Input tab? Stay on Log for now.
         m_generateGraphStaticText->SetLabel(_("Error loading results. Check log."));
    }
}
void DynaRangeFrame::OnWorkerUpdate(wxThreadEvent& event) {
    // Append log message received from worker thread to the log control
    wxString log_message = event.GetString();
    if(m_logController) {
        m_logController->AppendText(log_message);
    }
}


// =============================================================================
// FILE DROP TARGET IMPLEMENTATION
// =============================================================================

bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) {
    // Pass the dropped filenames to the InputController via the owner frame
    if (m_owner && m_owner->GetInputController()) { // Use getter
        m_owner->GetInputController()->AddDroppedFiles(filenames);
    }
    return true; // Indicate success
}