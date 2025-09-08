// Fichero: gui/DynaRangeFrame.cpp
#include "DynaRangeFrame.hpp"
#include "../core/Engine.hpp"
#include "../core/Analysis.hpp"

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

// --- LÓGICA DE LOGGING Y EVENTOS ---
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

// --- IMPLEMENTACIÓN DE LA VENTANA ---

DynaRangeFrame::DynaRangeFrame(wxWindow* parent) : MyFrameBase(parent)
{
    // --- Conexiones de Eventos (Binding) ---
    m_executeButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Bind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    m_cvsGrid->Bind(wxEVT_GRID_CELL_LEFT_CLICK, &DynaRangeFrame::OnGridCellClick, this);
    Bind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Bind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    m_darkFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Bind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    UpdateCommandPreview();
}

DynaRangeFrame::~DynaRangeFrame()
{
    // Desconectamos los eventos para evitar problemas al cerrar
    m_executeButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    m_cvsGrid->Unbind(wxEVT_GRID_CELL_LEFT_CLICK, &DynaRangeFrame::OnGridCellClick, this);
    Unbind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Unbind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    m_darkFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
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

    std::thread worker_thread([this]() {
        // Prepara el stream de log para la GUI, que captura la salida en tiempo real
        WxLogStreambuf log_streambuf(this);
        std::ostream log_stream(&log_streambuf);
        
        ProgramOptions opts = m_lastRunOptions;
        m_summaryPlotPath.clear();

        // Llama al motor centralizado, que ahora se encarga de toda la lógica y la salida
        auto summary_path_opt = RunDynamicRangeAnalysis(opts, log_stream);
        
        if(summary_path_opt) {
            m_summaryPlotPath = *summary_path_opt;
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
    
    if (m_summaryPlotPath.empty()) {
        AppendLog(_("\n---\nExecution failed. Please check the log for details."));
        wxMessageBox(_("An error occurred during processing. Please check the log tab for details."), _("Error"), wxOK | wxICON_ERROR, this);
    } else {
        AppendLog(_("\n---\nExecution finished successfully."));
        m_mainNotebook->SetSelection(2);
        LoadResults(m_lastRunOptions);
        LoadGraphImage(wxString(m_summaryPlotPath));
    }
}

void DynaRangeFrame::OnAddFilesClick(wxCommandEvent& event) {
    wxFileDialog openFileDialog(this, _("Select RAW files"), "", "", "RAW files (*.dng;*.cr2;*.nef;*.orf;*.arw)|*.dng;*.cr2;*.nef;*.orf;*.arw|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (openFileDialog.ShowModal() == wxID_CANCEL) return;
    openFileDialog.GetPaths(m_inputFiles);
    m_rawFileslistBox->Set(m_inputFiles);
    UpdateCommandPreview();
}

void DynaRangeFrame::OnGridCellClick(wxGridEvent& event)
{
    int row = event.GetRow();
    
    if (row < 1) { 
        if (!m_summaryPlotPath.empty()) {
            LoadGraphImage(wxString(m_summaryPlotPath));
        }
    } else { 
        wxString rawFilename = m_cvsGrid->GetCellValue(row, 0);
        if (!rawFilename.IsEmpty()) {
            LoadGraphImage(rawFilename);
        }
    }
    event.Skip();
}

void DynaRangeFrame::OnInputChanged(wxEvent& event) {
    UpdateCommandPreview();
}

ProgramOptions DynaRangeFrame::GetProgramOptions() {
    ProgramOptions opts;
    opts.dark_file_path = std::string(m_darkFilePicker->GetPath().mb_str());
    opts.sat_file_path = std::string(m_saturationFilePicker->GetPath().mb_str());
    m_darkValueTextCtrl->GetValue().ToDouble(&opts.dark_value);
    m_saturationValueTextCtrl->GetValue().ToDouble(&opts.saturation_value);
    
    opts.snr_threshold_db = 12.0;
    opts.dr_normalization_mpx = 8.0;
    opts.poly_order = 2; // Fijado a 2 para que las etiquetas EV funcionen
    opts.patch_safe = 50;
    
    for (const wxString& file : m_inputFiles) {
        opts.input_files.push_back(std::string(file.mb_str()));
    }

    wxString docsPath = wxStandardPaths::Get().GetDocumentsDir();
    fs::path output_dir = fs::path(std::string(docsPath.mb_str())); //   Removed-> / "DynaRange_Results";
    // Removed -> fs::create_directories(output_dir);
    opts.output_filename = (output_dir / "DR_results.csv").string();
    
    return opts;
}

void DynaRangeFrame::UpdateCommandPreview() {
    ProgramOptions opts = GetProgramOptions();
    std::stringstream command_ss;
    command_ss << "dynaRange";

    if (!opts.dark_file_path.empty()) {
        command_ss << " --black-file \"" << opts.dark_file_path << "\"";
    } else {
        command_ss << " --black-level " << opts.dark_value;
    }

    if (!opts.sat_file_path.empty()) {
        command_ss << " --saturation-file \"" << opts.sat_file_path << "\"";
    } else {
        command_ss << " --saturation-level " << opts.saturation_value;
    }

    command_ss << " --snrthreshold-db " << std::fixed << std::setprecision(2) << opts.snr_threshold_db;
    command_ss << " --poly-fit " << opts.poly_order;
    command_ss << " --drnormalization-mpx " << std::fixed << std::setprecision(2) << opts.dr_normalization_mpx;
    command_ss << " --patch-safe " << opts.patch_safe;
    command_ss << " --output-file \"" << opts.output_filename << "\"";
    
    command_ss << " --input-files";
    for (const auto& file : opts.input_files) {
        command_ss << " \"" << file << "\"";
    }
    
    m_equivalentCliTextCtrl->ChangeValue(command_ss.str());
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

void DynaRangeFrame::LoadResults(const ProgramOptions& opts) {
    std::ifstream file(opts.output_filename);
    if (!file) { return; }
    if (m_cvsGrid->GetNumberRows() > 0) m_cvsGrid->DeleteRows(0, m_cvsGrid->GetNumberRows());
    if (m_cvsGrid->GetNumberCols() > 0) m_cvsGrid->DeleteCols(0, m_cvsGrid->GetNumberCols());
    std::string line;
    int row = 0;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        int col = 0;
        if (row == 0) { // Cabecera
             while (std::getline(ss, cell, ',')) {
                if (col >= m_cvsGrid->GetNumberCols()) m_cvsGrid->AppendCols(1);
                m_cvsGrid->SetColLabelValue(col, cell);
                col++;
            }
        } else { // Datos
            m_cvsGrid->AppendRows(1);
            while (std::getline(ss, cell, ',')) {
                if (col >= m_cvsGrid->GetNumberCols()) m_cvsGrid->AppendCols(1);
                m_cvsGrid->SetCellValue(row - 1, col, cell);
                col++;
            }
        }
        row++;
    }
    m_cvsGrid->AutoSize();
    this->Layout();
}

void DynaRangeFrame::LoadGraphImage(const wxString& path_or_raw_name)
{
    if (path_or_raw_name.IsEmpty() || !m_imageGraph) {
        return;
    }

    fs::path graphPath;
    std::string displayFilename;
    fs::path inputPath(std::string(path_or_raw_name.mb_str()));
    fs::path outputDir = fs::path(m_lastRunOptions.output_filename).parent_path();

    if (inputPath.is_absolute() && fs::exists(inputPath)) {
        graphPath = inputPath;
    } else {
        graphPath = outputDir / (inputPath.stem().string() + "_snr_plot.png");
    }
    displayFilename = graphPath.filename().string();

    wxImage image;
    if (!fs::exists(graphPath) || !image.LoadFile(wxString(graphPath.string()))) {
        m_imageGraph->SetBitmap(wxBitmap(1,1));
        m_generateGraphStaticText->SetLabel(_("Generated Graph (Image not found): ") + wxString(displayFilename));
        return;
    }

    m_generateGraphStaticText->SetLabel(_("Generated Graph: ") + wxString(displayFilename));

    wxSize panelSize = m_imageGraph->GetSize();
    if (panelSize.GetWidth() <= 0 || panelSize.GetHeight() <= 0) return;

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