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
        
    // --- Bind Events ---
    m_executeButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    m_cvsGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &DynaRangeFrame::OnGridCellClick, this);
    m_darkFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_patchRatioSlider->Bind(wxEVT_SCROLL_THUMBTRACK, &DynaRangeFrame::OnPatchRatioSliderChanged, this);

    // Thread communication events
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    
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
    wxFileDialog openFileDialog(this, _("Select RAW files"), "", "", "RAW files (*.dng;*.cr2;*.nef;*.orf;*.arw)|*.dng;*.cr2;*.nef;*.orf;*.arw|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
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

void DynaRangeFrame::OnWorkerCompleted(wxCommandEvent& event) {
    SetUiState(false); // Go to "results" mode
    
    // Get final data from the presenter
    const ProgramOptions& final_opts = m_presenter->GetLastRunOptions();
    const ReportOutput& report = m_presenter->GetLastReport();

    // Update the UI with the results
    DisplayResults(final_opts.output_filename);

    if (report.summary_plot_path.has_value()) {
        LoadGraphImage(*report.summary_plot_path);
    } else {
        // Handle case where analysis finished but plot failed to generate
        AppendLog("\nError: Summary plot could not be generated.");
        LoadLogoImage(); // Revert to logo
        m_generateGraphStaticText->SetLabel(_("Results loaded, but summary plot failed."));
    }
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
        ShowError("Error", "Could not open the results file: " + csv_path);
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

void DynaRangeFrame::ShowError(const std::string& title, const std::string& message) {
    wxMessageBox(message, title, wxOK | wxICON_ERROR, this);
}

void DynaRangeFrame::SetUiState(bool is_processing) {
    m_executeButton->Enable(!is_processing);
    m_csvOutputStaticText->Show(!is_processing);
    m_cvsGrid->Show(!is_processing);
    m_processingGauge->Show(is_processing);

    if (is_processing) {
        m_mainNotebook->SetSelection(1); // Switch to log tab
        ClearLog();
        m_gaugeTimer->Start(100);
        LoadLogoImage();
        m_generateGraphStaticText->SetLabel(_("Processing... Please wait."));
    } else {
        m_gaugeTimer->Stop();
    }
    m_resultsPanel->Layout();
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