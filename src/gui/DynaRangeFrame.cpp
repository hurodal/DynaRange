// File: gui/DynaRangeFrame.cpp
/**
 * @file DynaRangeFrame.cpp
 * @brief Implementation of the DynaRange GUI's main frame (the View).
 * @author Juanma Font
 * @date 2025-09-10
 */
#include "DynaRangeFrame.hpp"
#include <libraw/libraw.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm> // For std::count

namespace fs = std::filesystem;

// --- EVENT DEFINITIONS ---
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxCommandEvent);

// --- FileDropTarget IMPLEMENTATION ---
bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) {
    if (m_owner) {
        m_owner->AddDroppedFiles(filenames);
    }
    return true;
}

// --- MAIN FRAME IMPLEMENTATION ---

DynaRangeFrame::DynaRangeFrame(wxWindow* parent) : MyFrameBase(parent)
{
    // --- Create the Presenter ---
    m_presenter = std::make_unique<GuiPresenter>(this);

    // --- UI Initialization ---
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fn(exePath);
    wxString appDir = fn.GetPath();
    #ifdef __WXMSW__
        SetIcon(wxIcon("MAINICON", wxBITMAP_TYPE_ICO_RESOURCE));
    #else
        wxString iconPath = appDir + wxFILE_SEP_PATH + "favicon_noise.ico";
        if (wxFileExists(iconPath)) { SetIcon(wxIcon(iconPath, wxBITMAP_TYPE_ICO)); }
    #endif

    // Force font to 9    
    // 1. Obtenemos la fuente por defecto del sistema
    //wxFont defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    // 2. Le cambiamos el tamaño
    //defaultFont.SetPointSize(9);
    // 3. Aplicamos la nueva fuente (con tamaño modificado) a toda la ventana
    //this->SetFont(defaultFont);
        
    // --- Bind Events ---
    m_executeButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    m_cvsGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &DynaRangeFrame::OnGridCellClick, this);
    m_darkFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_patchRatioSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &DynaRangeFrame::OnPatchRatioSliderChanged, this);
    m_removeRawFilesButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnRemoveFilesClick, this);
    m_rawFileslistBox->Bind(wxEVT_LISTBOX, &DynaRangeFrame::OnListBoxSelectionChanged, this);
    m_rawFileslistBox->Bind(wxEVT_KEY_DOWN, &DynaRangeFrame::OnListBoxKeyDown, this);
    m_snrThresholdslider->Bind(wxEVT_SCROLL_THUMBTRACK, &DynaRangeFrame::OnSnrSliderChanged, this);
    m_drNormalizationSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &DynaRangeFrame::OnDrNormSliderChanged, this);
    m_PlotChoice->Bind(wxEVT_CHOICE, &DynaRangeFrame::OnInputChanged, this);
    m_plotingChoice->Bind(wxEVT_CHOICE, &DynaRangeFrame::OnInputChanged, this);
    m_outputTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    //m_cliCollapsiblePane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, &DynaRangeFrame::OnCliPaneChanged, this);
  
    // Thread communication events
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    Bind(wxEVT_CLOSE_WINDOW, &DynaRangeFrame::OnClose, this);
    
    // Gauge animation timer
    m_gaugeTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &DynaRangeFrame::OnGaugeTimer, this, m_gaugeTimer->GetId());
    
    // Drag and Drop
    m_dropTarget = new FileDropTarget(this);
    SetDropTarget(m_dropTarget);

    // --- Initial State Setup ---
    m_cvsGrid->Show(false);
    m_csvOutputStaticText->Show(false);
    LoadLogoImage();
    m_presenter->UpdateCommandPreview();
}

DynaRangeFrame::~DynaRangeFrame() {
    // Unbinding is handled automatically by wxWidgets for member functions
    delete m_gaugeTimer;
}

// --- Event Handlers (Delegation to Presenter) ---

void DynaRangeFrame::OnExecuteClick(wxCommandEvent& event) {
    m_presenter->StartAnalysis();
}

void DynaRangeFrame::OnAddFilesClick(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, _("Select RAW files"), "", "", _("RAW files (*.dng;*.cr2;*.nef;*.orf;*.arw)|*.dng;*.cr2;*.nef;*.orf;*.arw|All files (*.*)|*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (openFileDialog.ShowModal() == wxID_CANCEL) { return; }
    
    wxArrayString paths;
    openFileDialog.GetPaths(paths);
    AddDroppedFiles(paths);
}

void DynaRangeFrame::OnGridCellClick(wxGridEvent& event) {
    m_presenter->HandleGridCellClick(event.GetRow());
    event.Skip();
}

void DynaRangeFrame::OnInputChanged(wxEvent& event) {
    m_presenter->UpdateCommandPreview();
}

void DynaRangeFrame::OnPatchRatioSliderChanged(wxScrollEvent& event) {
    // 1. Obtiene el valor actual del slider (que es un float 0.0-1.0)
    double value = GetPatchRatio();

    // 2. Actualiza la etiqueta de texto al lado del slider
    m_patchRatioValueText->SetLabel(wxString::Format("%.2f", value));

    // 3. Notifica al Presenter para que actualice el command preview
    m_presenter->UpdateCommandPreview();
}

// --- Thread Communication Event Handlers ---

void DynaRangeFrame::OnWorkerUpdate(wxThreadEvent& event) {
    AppendLog(event.GetString());
}

// File: src/gui/DynaRangeFrame.cpp

void DynaRangeFrame::OnWorkerCompleted(wxCommandEvent& event) {
    SetUiState(false); // Go to "results" mode
    
    // Get final data from the presenter
    const ProgramOptions& final_opts = m_presenter->GetLastRunOptions();
    const ReportOutput& report = m_presenter->GetLastReport();

    // Update the UI with the results, using the definitive path from the report.
    DisplayResults(report.final_csv_path);

    if (report.summary_plot_path.has_value()) {
        // If a plot path exists, load the image.
        LoadGraphImage(*report.summary_plot_path);
    } else {
        // If no plot path, check why.
        if (final_opts.plot_mode == 0) {
            // Case 1: The user chose not to generate a plot.
            LoadLogoImage(); // Revert to logo
            m_generateGraphStaticText->SetLabel(_("Results loaded. Plot generation was not requested."));
        } else {
            // Case 2: A plot was expected but failed to generate.
            AppendLog(_("\nError: Summary plot could not be generated."));
            LoadLogoImage(); // Revert to logo
            m_generateGraphStaticText->SetLabel(_("Results loaded, but summary plot failed."));
        }
    }
}

void DynaRangeFrame::OnClose(wxCloseEvent& event) {
    if (m_presenter->IsWorkerRunning()) {
        m_presenter->RequestWorkerCancellation(); // Envía la señal de cancelación
        // El destructor se encargará de esperar (join) al hilo, que ahora terminará pronto.
    }
    Destroy(); // Le dice a la ventana que se destruya
}

void DynaRangeFrame::PostLogUpdate(const std::string& text) {
    wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_WORKER_UPDATE);
    event->SetString(text);
    wxQueueEvent(this, event);
}

void DynaRangeFrame::PostAnalysisComplete() {
    wxQueueEvent(this, new wxCommandEvent(wxEVT_COMMAND_WORKER_COMPLETED));
}

// --- Methods Called by the Presenter to Update the View ---

void DynaRangeFrame::UpdateInputFileList(const std::vector<std::string>& files) {
    wxArrayString wx_files;
    for (const auto& file : files) {
        wx_files.Add(file);
    }
    m_rawFileslistBox->Set(wx_files);
}

void DynaRangeFrame::UpdateCommandPreview(const std::string& command) {
    m_equivalentCliTextCtrl->ChangeValue(command);
}

void DynaRangeFrame::DisplayResults(const std::string& csv_path) {
    std::ifstream file(csv_path);
    if (!file) { 
        ShowError(_("Error"), _("Could not open the results file: ") + csv_path);
        return;
    }

    if (m_cvsGrid->GetNumberRows() > 0) m_cvsGrid->DeleteRows(0, m_cvsGrid->GetNumberRows());
    if (m_cvsGrid->GetNumberCols() > 0) m_cvsGrid->DeleteCols(0, m_cvsGrid->GetNumberCols());

    std::string line;
    if (std::getline(file, line)) { // Read header
        std::stringstream ss(line);
        std::string cell;
        int num_cols = std::count(line.begin(), line.end(), ',') + 1;
        m_cvsGrid->AppendCols(num_cols);

        for(int i = 0; i < num_cols; ++i) m_cvsGrid->SetColLabelValue(i, "");
        m_cvsGrid->SetColLabelSize(WXGRID_DEFAULT_COL_LABEL_HEIGHT);
        m_cvsGrid->AppendRows(1); // Row for header
        int col = 0;
        while (std::getline(ss, cell, ',')) {
            m_cvsGrid->SetCellValue(0, col, cell);
            m_cvsGrid->SetReadOnly(0, col++, true);
        }
    }

    while (std::getline(file, line)) { // Read data rows
        std::stringstream ss(line);
        std::string cell;
        m_cvsGrid->AppendRows(1);
        int grid_row = m_cvsGrid->GetNumberRows() - 1;
        int col = 0;
        while (std::getline(ss, cell, ',')) {
            m_cvsGrid->SetCellValue(grid_row, col, cell);
            m_cvsGrid->SetReadOnly(grid_row, col++, true);
        }
    }

    m_cvsGrid->AutoSize();
    m_mainNotebook->SetSelection(2);
    m_resultsPanel->Layout();
}

void DynaRangeFrame::ShowError(const wxString& title, const wxString& message) {
    wxMessageBox(message, title, wxOK | wxICON_ERROR, this);
}

void DynaRangeFrame::SetUiState(bool is_processing) {
    m_executeButton->Enable(!is_processing);

    if (is_processing) {
        // --- Estado "Procesando" ---
        m_mainNotebook->SetSelection(1); // Cambia a la pestaña de Log
        ClearLog();
        // Oculta explícitamente los controles de resultados
        m_csvOutputStaticText->Hide();
        m_cvsGrid->Hide();
        // Muestra los controles de "procesando" y recarga el logo
        m_generateGraphStaticText->SetLabel(_("Processing... Please wait."));
        LoadLogoImage(); // Recargar el logo
        m_processingGauge->Show();
        m_gaugeTimer->Start(100);
    } else {
        // --- Estado "Resultados" ---
        m_gaugeTimer->Stop();
        m_processingGauge->Hide();

        // Actualiza la etiqueta de texto a su estado final ANTES de mostrarla
        m_generateGraphStaticText->SetLabel(_("Generated Graph:"));
        // Muestra explícitamente todos los controles de resultados
        m_csvOutputStaticText->Show();
        m_cvsGrid->Show();
        m_imageGraph->Show();
    }

    // Finalmente, fuerza al panel a recalcular el layout y a redibujarse
    m_resultsPanel->Layout();
    m_resultsPanel->Refresh(); // Esta línea es clave para Windows
}


// --- Getters for the Presenter ---

std::string DynaRangeFrame::GetDarkFilePath() const {
    return std::string(m_darkFilePicker->GetPath().mb_str());
}
std::string DynaRangeFrame::GetSaturationFilePath() const {
    return std::string(m_saturationFilePicker->GetPath().mb_str());
}
double DynaRangeFrame::GetDarkValue() const {
    double val;
    m_darkValueTextCtrl->GetValue().ToDouble(&val);
    return val;
}
double DynaRangeFrame::GetSaturationValue() const {
    double val;
    m_saturationValueTextCtrl->GetValue().ToDouble(&val);
    return val;
}

double DynaRangeFrame::GetPatchRatio() const {
    // Lee el valor entero del slider (0-100) y lo convierte a un flotante (0.0-1.0)
    return static_cast<double>(m_patchRatioSlider->GetValue()) / 100.0;
}

std::string DynaRangeFrame::GetOutputFilePath() const {
    return std::string(m_outputTextCtrl->GetValue().mb_str());
}

double DynaRangeFrame::GetSnrThreshold() const {
    // The slider value is an integer, so we cast it to double.
    return static_cast<double>(m_snrThresholdslider->GetValue());
}

double DynaRangeFrame::GetDrNormalization() const {
    // The slider value is an integer, so we cast it to double.
    return static_cast<double>(m_drNormalizationSlider->GetValue());
}

int DynaRangeFrame::GetPolyOrder() const {
    // The choice index (0 or 1) maps to the polynomial order (2 or 3).
    return m_PlotChoice->GetSelection() + 2;
}

int DynaRangeFrame::GetPlotMode() const {
    // The choice index now directly corresponds to the plot mode value (0, 1, 2, 3)
    return m_plotingChoice->GetSelection();
}

// --- Other UI and Helper Functions ---

void DynaRangeFrame::AddDroppedFiles(const wxArrayString& filenames) {
    wxArrayString supported_files;
    wxArrayString rejected_files;

    for (const auto& file : filenames) {
        if (IsSupportedRawFile(file)) {
            supported_files.Add(file);
        } else {
            rejected_files.Add(wxFileName(file).GetFullName());
        }
    }

    if (!supported_files.IsEmpty()) {
        std::vector<std::string> files_to_add;
        for (const auto& f : supported_files) {
            files_to_add.push_back(std::string(f.mb_str()));
        }
        m_presenter->AddInputFiles(files_to_add);
    }
    
    if (!rejected_files.IsEmpty()) {
        wxString message = _("The following files were ignored because they are not recognized as supported RAW formats:\n\n");
        for (const auto& rejected : rejected_files) { message += "- " + rejected + "\n"; }
        wxMessageBox(message, _("Unsupported Files Skipped"), wxOK | wxICON_INFORMATION, this);
    }
}

void DynaRangeFrame::OnGaugeTimer(wxTimerEvent& event) { m_processingGauge->Pulse(); }
void DynaRangeFrame::ClearLog() { m_logOutputTextCtrl->Clear(); }
void DynaRangeFrame::AppendLog(const wxString& text) { m_logOutputTextCtrl->AppendText(text); }

void DynaRangeFrame::LoadGraphImage(const std::string& image_path) {
    if (image_path.empty() || !m_imageGraph) return;
    
    fs::path graphPath(image_path);
    std::string displayFilename = graphPath.filename().string();
    
    wxImage image;

    if (!fs::exists(graphPath) || !image.LoadFile(wxString(graphPath.string()))) {
        m_imageGraph->SetBitmap(wxBitmap());
        m_generateGraphStaticText->SetLabel(_("Generated Graph (Image not found): ") + wxString(displayFilename));
    } else {
        m_generateGraphStaticText->SetLabel(_("Generated Graph: ") + wxString(displayFilename));
        wxSize panelSize = m_imageGraph->GetSize();
        if (panelSize.GetWidth() <= 0 || panelSize.GetHeight() <= 0) {
            m_resultsPanel->Layout();
            panelSize = m_imageGraph->GetSize();
        }
        if (panelSize.GetWidth() > 0 && panelSize.GetHeight() > 0) {
            int imgWidth = image.GetWidth();
            int imgHeight = image.GetHeight();
            double hScale = (double)panelSize.GetWidth() / imgWidth;
            double vScale = (double)panelSize.GetHeight() / imgHeight;
            double scale = std::min(hScale, vScale);
            if (scale < 1.0) {
                image.Rescale(imgWidth * scale, imgHeight * scale, wxIMAGE_QUALITY_HIGH);
            }
        }
        m_imageGraph->SetBitmap(wxBitmap(image));
    }
    m_resultsPanel->Layout();
}

void DynaRangeFrame::LoadLogoImage() {
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fn(exePath);
    wxString appDir = fn.GetPath();
    wxString logoPath = appDir + wxFILE_SEP_PATH + "logo.png";
    wxImage logoImage;
    if (logoImage.LoadFile(logoPath, wxBITMAP_TYPE_PNG)) {
        m_imageGraph->SetBitmap(wxBitmap(logoImage));
        m_generateGraphStaticText->SetLabel(_("Welcome to Dynamic Range Calculator"));
    } else {
        m_imageGraph->SetBitmap(wxBitmap());
        m_generateGraphStaticText->SetLabel(_("Welcome (logo.png not found)"));
    }
    m_resultsPanel->Layout();
}

bool DynaRangeFrame::IsSupportedRawFile(const wxString& filePath) {
    LibRaw p;
    return p.open_file(filePath.mb_str()) == LIBRAW_SUCCESS;
}

void DynaRangeFrame::PerformFileRemoval() {
    wxArrayInt selections;
    if (m_rawFileslistBox->GetSelections(selections) == 0) return;

    // Convertimos wxArrayInt a std::vector<int> para el presenter
    std::vector<int> indices(selections.begin(), selections.end());
    m_presenter->RemoveInputFiles(indices);

    // Después de que los ficheros se han eliminado, nos aseguramos
    // de que el botón se deshabilite, ya que la selección se ha perdido.
    m_removeRawFilesButton->Enable(false);
}

void DynaRangeFrame::OnRemoveFilesClick(wxCommandEvent& event) {
    PerformFileRemoval();
}

void DynaRangeFrame::OnListBoxSelectionChanged(wxCommandEvent& event) {
    // Habilita o deshabilita el botón "Remove" según si hay algo seleccionado
    wxArrayInt selections;
    m_removeRawFilesButton->Enable(m_rawFileslistBox->GetSelections(selections) > 0);
    event.Skip();
}

void DynaRangeFrame::OnListBoxKeyDown(wxKeyEvent& event) {
    // Implementa la eliminación con la tecla Suprimir/Retroceso
    if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_BACK) {
        PerformFileRemoval(); // Reutilizamos la nueva función
    }
    event.Skip();
}

void DynaRangeFrame::OnSnrSliderChanged(wxScrollEvent& event) {
    double value = GetSnrThreshold();
    m_snrThresholdValueText->SetLabel(wxString::Format("%.0fdB", value));
    m_presenter->UpdateCommandPreview();
}

void DynaRangeFrame::OnDrNormSliderChanged(wxScrollEvent& event) {
    double value = GetDrNormalization();
    m_drNormalizationValueText->SetLabel(wxString::Format("%.0fMpx", value));
    m_presenter->UpdateCommandPreview();
}

//void DynaRangeFrame::OnCliPaneChanged(wxCollapsiblePaneEvent& event)
//{
//    // When the pane's state changes, we need to tell the parent panel's sizer
//    // to recalculate the layout of all its children. This will move the button up/down.
//    m_inputPanel->Layout();
//
//    // It is good practice to allow the event to propagate if needed.
//    event.Skip();
//}