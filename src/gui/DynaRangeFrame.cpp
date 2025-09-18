/**
 * @file DynaRangeFrame.cpp
 * @brief Implementation of the DynaRange GUI's main frame.
 * @author Juanma Font
 * @date 2025-09-10
 */
#include "DynaRangeFrame.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/arguments/CommandGenerator.hpp"
#include <libraw/libraw.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <thread>
#include <ostream>
#include <streambuf>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// --- LOGGING AND EVENT LOGIC ---
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_WORKER_COMPLETED, wxThreadEvent);

class WxLogStreambuf : public std::streambuf {
public:
    WxLogStreambuf(wxEvtHandler* target) : m_target(target) {}
protected:
    virtual int sync() override {
        if (!m_buffer.empty()) {
            wxThreadEvent* event = new wxThreadEvent(wxEVT_COMMAND_WORKER_UPDATE);
            event->SetString(m_buffer);
            wxQueueEvent(m_target, event);
            m_buffer.clear();
        }
        return 0;
    }
    virtual int overflow(int c) override {
        if (c != EOF) {
            m_buffer += static_cast<char>(c);
            if (c == '\n') { sync(); }
        }
        return c;
    }
private:
    wxEvtHandler* m_target;
    std::string m_buffer;
};


bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    if (m_owner) {
        m_owner->AddDroppedFiles(filenames);
    }
    return true;
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

// --- MAIN FRAME IMPLEMENTATION ---

DynaRangeFrame::DynaRangeFrame(wxWindow* parent) : MyFrameBase(parent)
{
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fn(exePath);
    wxString appDir = fn.GetPath();
    
    #ifdef __WXMSW__
        SetIcon(wxIcon("MAINICON", wxBITMAP_TYPE_ICO_RESOURCE));
    #else
        wxString iconPath = appDir + wxFILE_SEP_PATH + "favicon_noise.ico";
        if (wxFileExists(iconPath)) {
            SetIcon(wxIcon(iconPath, wxBITMAP_TYPE_ICO));
        }
    #endif
        
    m_executeButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    m_cvsGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &DynaRangeFrame::OnGridCellClick, this);
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    m_darkFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);

    m_gaugeTimer = new wxTimer(this, wxID_ANY);
    Bind(wxEVT_TIMER, &DynaRangeFrame::OnGaugeTimer, this, m_gaugeTimer->GetId());
    
    m_dropTarget = new FileDropTarget(this);
    SetDropTarget(m_dropTarget);

    UpdateCommandPreview();

    // Oculta el grid de datos hasta que tenga datos.
    m_cvsGrid->Show(false);
    m_csvOutputStaticText->Show(false);
    
    LoadLogoImage(); // Carga el logo y texto de bienvenida
}

DynaRangeFrame::~DynaRangeFrame()
{
    m_executeButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    m_cvsGrid->Unbind(wxEVT_GRID_CELL_LEFT_CLICK, &DynaRangeFrame::OnGridCellClick, this);
    Unbind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Unbind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    m_darkFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    Unbind(wxEVT_TIMER, &DynaRangeFrame::OnGaugeTimer, this, m_gaugeTimer->GetId());
    delete m_gaugeTimer;

    SetDropTarget(nullptr);
}

void DynaRangeFrame::OnExecuteClick(wxCommandEvent& event)
{
    m_lastRunOptions = GetProgramOptions();
    if (m_lastRunOptions.input_files.empty()) {
        wxMessageBox(_("Please select at least one input RAW file."), _("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    SetExecuteButtonState(false);
    m_mainNotebook->SetSelection(1);
    ClearLog();
    
    SetResultsPanelState(true);

    if (m_cvsGrid->GetNumberRows() > 0) m_cvsGrid->DeleteRows(0, m_cvsGrid->GetNumberRows());
    if (m_cvsGrid->GetNumberCols() > 0) m_cvsGrid->DeleteCols(0, m_cvsGrid->GetNumberCols());

    std::thread worker_thread([this]() {
        WxLogStreambuf log_streambuf(this);
        std::ostream log_stream(&log_streambuf);
        
        ProgramOptions opts = m_lastRunOptions;
        
        m_summaryPlotPath.clear();
        m_individualPlotPaths.clear();

        ReportOutput report = DynaRange::RunDynamicRangeAnalysis(opts, log_stream); // ← CORREGIDO
        
        if(report.summary_plot_path) {
            m_summaryPlotPath = *report.summary_plot_path;
            m_individualPlotPaths = report.individual_plot_paths;
        }

        wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED));
    });

    worker_thread.detach();
}

void DynaRangeFrame::OnWorkerUpdate(wxThreadEvent& event) {
    AppendLog(event.GetString());
}

void DynaRangeFrame::OnWorkerCompleted(wxThreadEvent& event) {
    SetExecuteButtonState(true);
    SetResultsPanelState(false);
    
    if (m_summaryPlotPath.empty()) {
        AppendLog(_("\n---\nExecution failed. Please check the log for details."));
        wxMessageBox(_("An error occurred during processing. Please check the log tab for details."), _("Error"), wxOK | wxICON_ERROR, this);
        m_generateGraphStaticText->SetLabel(_("Error during processing. Check log."));
    } else {
        AppendLog(_("\n---\nExecution finished successfully."));
        LoadResults(m_lastRunOptions);
        LoadGraphImage(wxString(m_summaryPlotPath));
        m_mainNotebook->SetSelection(2);
    }
}

void DynaRangeFrame::OnGridCellClick(wxGridEvent& event)
{
    int row = event.GetRow();

    // La fila 0 es ahora la cabecera "falsa" para el gráfico de resumen
    if (row == 0) { 
        if (!m_summaryPlotPath.empty()) {
            LoadGraphImage(wxString(m_summaryPlotPath));
        }
    } 
    // Las filas de datos ahora empiezan en el índice 1
    else if (row >= 1) { 
        wxString rawFilenameGrid = m_cvsGrid->GetCellValue(row, 0);
        if (!rawFilenameGrid.IsEmpty()) {
            std::string rawFilenameToShow;
            // Busca el nombre de fichero completo que corresponde a la celda
            for(const auto& path_str : m_lastRunOptions.input_files){
                if (fs::path(path_str).filename().string() == std::string(rawFilenameGrid.mb_str())) {
                    rawFilenameToShow = path_str;
                    break;
                }
            }

            if (!rawFilenameToShow.empty() && m_individualPlotPaths.count(rawFilenameToShow)) {
                wxString plotPath = m_individualPlotPaths.at(rawFilenameToShow);
                LoadGraphImage(plotPath);
            }
        }
    }
    // Clics en las cabeceras nativas (-1) no hacen nada.
    
    event.Skip();
}

void DynaRangeFrame::OnInputChanged(wxEvent& event) {
    UpdateCommandPreview();
}

void DynaRangeFrame::LoadResults(const ProgramOptions& opts)
{
    std::ifstream file(opts.output_filename);
    if (!file) { return; }

    // Limpieza completa del grid
    if (m_cvsGrid->GetNumberRows() > 0) m_cvsGrid->DeleteRows(0, m_cvsGrid->GetNumberRows());
    if (m_cvsGrid->GetNumberCols() > 0) m_cvsGrid->DeleteCols(0, m_cvsGrid->GetNumberCols());

    std::string line;
    bool is_header_line = true;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        int col = 0;

        if (is_header_line) {
            int num_cols = std::count(line.begin(), line.end(), ',') + 1;
            m_cvsGrid->AppendCols(num_cols);

            // Poner las cabeceras nativas (grises) en blanco
            for(int i = 0; i < num_cols; ++i) {
                m_cvsGrid->SetColLabelValue(i, "");
            }
            
            m_cvsGrid->SetColLabelSize(WXGRID_DEFAULT_COL_LABEL_HEIGHT);

            // La fila 0 contiene ahora los títulos
            m_cvsGrid->AppendRows(1); 
            while (std::getline(ss, cell, ',')) {
                m_cvsGrid->SetCellValue(0, col, cell); // Escribir títulos en la fila 0
                m_cvsGrid->SetReadOnly(0, col, true);
                col++;
            }
            is_header_line = false;
        } else {
            m_cvsGrid->AppendRows(1);
            int grid_row = m_cvsGrid->GetNumberRows() - 1; // Fila de datos actual
            while (std::getline(ss, cell, ',')) {
                m_cvsGrid->SetCellValue(grid_row, col, cell);
                m_cvsGrid->SetReadOnly(grid_row, col, true);
                col++;
            }
        }
    }

    m_cvsGrid->AutoSize();
    m_resultsPanel->Layout();
}

ProgramOptions DynaRangeFrame::GetProgramOptions() {
    ProgramOptions opts;
    opts.dark_file_path = std::string(m_darkFilePicker->GetPath().mb_str());
    opts.sat_file_path = std::string(m_saturationFilePicker->GetPath().mb_str());
    m_darkValueTextCtrl->GetValue().ToDouble(&opts.dark_value);
    m_saturationValueTextCtrl->GetValue().ToDouble(&opts.saturation_value);
    opts.snr_thresholds_db = {12.0, 0.0};
    opts.dr_normalization_mpx = 8.0;
    opts.poly_order = 3;
    opts.patch_ratio = 0.5;
    opts.plot_mode = 2;
    for (const wxString& file : m_inputFiles) {
        opts.input_files.push_back(std::string(file.mb_str()));
    }
    wxString docsPath = wxStandardPaths::Get().GetDocumentsDir();
    fs::path output_dir = fs::path(std::string(docsPath.mb_str()));
    opts.output_filename = (output_dir / "DR_results.csv").string();
    return opts;
}

void DynaRangeFrame::UpdateCommandPreview() {
    ProgramOptions opts = GetProgramOptions();
    std::string command_string = GenerateCommand(opts);
    m_equivalentCliTextCtrl->ChangeValue(command_string);
}

void DynaRangeFrame::SetExecuteButtonState(bool enabled) {
    m_executeButton->Enable(enabled);
}

void DynaRangeFrame::ClearLog() {
    m_logOutputTextCtrl->Clear();
}

void DynaRangeFrame::AppendLog(const wxString& text) {
    m_logOutputTextCtrl->AppendText(text);
}

void DynaRangeFrame::LoadGraphImage(const wxString& path_or_raw_name)
{
    if (path_or_raw_name.IsEmpty() || !m_imageGraph) { return; }
    fs::path graphPath(std::string(path_or_raw_name.mb_str()));
    std::string displayFilename = graphPath.filename().string();
    wxImage image;
    if (!fs::exists(graphPath) || !image.LoadFile(wxString(graphPath.string()))) {
        m_imageGraph->SetBitmap(wxBitmap());
        m_generateGraphStaticText->SetLabel(_("Generated Graph (Image not found): ") + wxString(displayFilename));
        m_resultsPanel->Layout();
        return;
    }
    m_generateGraphStaticText->SetLabel(_("Generated Graph: ") + wxString(displayFilename));
    wxSize panelSize = m_imageGraph->GetSize();
    if (panelSize.GetWidth() <= 0 || panelSize.GetHeight() <= 0) {
        m_resultsPanel->Layout();
        panelSize = m_imageGraph->GetSize();
        if(panelSize.GetWidth() <= 0 || panelSize.GetHeight() <= 0) return;
    }
    int imgWidth = image.GetWidth();
    int imgHeight = image.GetHeight();
    double hScale = (double)panelSize.GetWidth() / imgWidth;
    double vScale = (double)panelSize.GetHeight() / imgHeight;
    double scale = std::min(hScale, vScale);
    if (scale < 1.0) {
        image.Rescale(imgWidth * scale, imgHeight * scale, wxIMAGE_QUALITY_HIGH);
    }
    m_imageGraph->SetBitmap(wxBitmap(image));
    m_resultsPanel->Layout();
}

bool DynaRangeFrame::IsSupportedRawFile(const wxString& filePath)
{
    LibRaw raw_processor;
    if (raw_processor.open_file(filePath.mb_str()) == LIBRAW_SUCCESS) {
        return true;
    }
    return false;
}

void DynaRangeFrame::AddRawFilesToList(const wxArrayString& paths)
{
    wxArrayString rejectedFiles;
    int addedCount = 0;
    for (const auto& file : paths) {
        if (IsSupportedRawFile(file)) {
            m_inputFiles.Add(file);
            addedCount++;
        } else {
            rejectedFiles.Add(wxFileName(file).GetFullName());
        }
    }
    if (addedCount > 0) {
        m_rawFileslistBox->Set(m_inputFiles);
        UpdateCommandPreview();
    }
    if (!rejectedFiles.IsEmpty()) {
        wxString message = _("The following files were ignored because they are not recognized as supported RAW formats:\n\n");
        for (const auto& rejected : rejectedFiles) {
            message += "- " + rejected + "\n";
        }
        wxMessageBox(message, _("Unsupported Files Skipped"), wxOK | wxICON_INFORMATION, this);
    }
}

void DynaRangeFrame::OnAddFilesClick(wxCommandEvent& event) 
{
    wxFileDialog openFileDialog(this, _("Select RAW files"), "", "", "RAW files (*.dng;*.cr2;*.nef;*.orf;*.arw)|*.dng;*.cr2;*.nef;*.orf;*.arw|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return;
    }
    wxArrayString paths;
    openFileDialog.GetPaths(paths);
    AddRawFilesToList(paths);
}

void DynaRangeFrame::AddDroppedFiles(const wxArrayString& filenames)
{
    AddRawFilesToList(filenames);
}

void DynaRangeFrame::SetResultsPanelState(bool processing)
{
    m_csvOutputStaticText->Show(!processing);
    m_cvsGrid->Show(!processing); // Mostrar/ocultar el grid según el estado
    m_processingGauge->Show(processing);
    if (processing) {
        m_gaugeTimer->Start(100);
        LoadLogoImage(); // Carga el logo y texto de procesamiento
        m_generateGraphStaticText->SetLabel(_("Processing... Please wait."));
    } else {
        m_gaugeTimer->Stop();
    }
    m_resultsPanel->Layout();
}

void DynaRangeFrame::OnGaugeTimer(wxTimerEvent& event)
{
    m_processingGauge->Pulse();
}