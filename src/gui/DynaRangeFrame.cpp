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
        WxLogStreambuf log_streambuf(this);
        std::ostream log_stream(&log_streambuf);
        
        ProgramOptions opts = m_lastRunOptions;

        if (!opts.dark_file_path.empty()) {
            auto dark_val_opt = ProcessDarkFrame(opts.dark_file_path, log_stream);
            if (!dark_val_opt) { m_summaryPlotPath.clear(); wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED)); return; }
            opts.dark_value = *dark_val_opt;
        }

        if (!opts.sat_file_path.empty()) {
            auto sat_val_opt = ProcessSaturationFrame(opts.sat_file_path, log_stream);
            if (!sat_val_opt) { m_summaryPlotPath.clear(); wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED)); return; }
            opts.saturation_value = *sat_val_opt;
        }

        if (!PrepareAndSortFiles(opts, log_stream)) {
            m_summaryPlotPath.clear(); wxQueueEvent(this, new wxThreadEvent(wxEVT_COMMAND_WORKER_COMPLETED)); return;
        }
        

        //  Captura la ruta del gráfico resumen
        auto summary_path_opt = RunDynamicRangeAnalysis(opts, log_stream);
        if(summary_path_opt) {
            m_summaryPlotPath = *summary_path_opt;
        } else {
            m_summaryPlotPath.clear();
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
    //  Comprueba si m_summaryPlotPath está vacío para determinar el éxito
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
    
    // Si se hace clic en la cabecera (fila 0), o en el área fuera de los datos.
    if (row < 1) { 
        if (!m_summaryPlotPath.empty()) {
            LoadGraphImage(wxString(m_summaryPlotPath)); // Carga el gráfico resumen
        }
    } else { // Si se hace clic en una fila de datos (RAW individual)
        // La columna 0 contiene el nombre del fichero RAW, ej: "iso00200.DNG"
        wxString rawFilename = m_cvsGrid->GetCellValue(row, 0); 
        if (!rawFilename.IsEmpty()) {
            LoadGraphImage(rawFilename); // Carga el gráfico individual para ese RAW
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
    
    // Valores fijos para la GUI (podrían añadirse controles más adelante)
    opts.snr_threshold_db = 12.0;
    opts.dr_normalization_mpx = 8.0;
    opts.poly_order = 2;
    opts.patch_safe = 50;
    
    for (const wxString& file : m_inputFiles) {
        opts.input_files.push_back(std::string(file.mb_str()));
    }

    wxString docsPath = wxStandardPaths::Get().GetDocumentsDir();
    // La variable output_dir se declara aquí
    fs::path output_dir = fs::path(std::string(docsPath.mb_str()));
    
    opts.output_filename = (output_dir / "DR_results.csv").string();
    return opts;
}
void DynaRangeFrame::UpdateCommandPreview() {
    wxString command = "dynaRange";
    if (!m_darkFilePicker->GetPath().IsEmpty()) { command += " --black-file \"" + m_darkFilePicker->GetPath() + "\""; }
    else { command += " --black-level " + m_darkValueTextCtrl->GetValue(); }
    if (!m_saturationFilePicker->GetPath().IsEmpty()) { command += " --saturation-file \"" + m_saturationFilePicker->GetPath() + "\""; }
    else { command += " --saturation-level " + m_saturationValueTextCtrl->GetValue(); }

    command += " --snrthreshold-db " + wxString::Format("%.2f", 12.0);
    command += " --poly-fit " + wxString::Format("%d", 2);
    command += " --drnormalization-mpx " + wxString::Format("%.2f", 8.0);
    command += " --patch-safe " + wxString::Format("%d", 50);
    
    wxString docsPath = wxStandardPaths::Get().GetDocumentsDir();
    fs::path output_dir = fs::path(std::string(docsPath.mb_str()));
    command += " --output-file \"" + wxString((output_dir / "DR_results.csv").string()) + "\"";

    command += " --input-files ";
    for (const wxString& file : m_inputFiles) { command += " \"" + file + "\""; }
    m_equivalentCliTextCtrl->ChangeValue(command);
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
        m_cvsGrid->AppendRows(1);
        while (std::getline(ss, cell, ',')) {
            if (col >= m_cvsGrid->GetNumberCols()) { m_cvsGrid->AppendCols(1); }
            m_cvsGrid->SetCellValue(row, col, cell);
            col++;
        }
        row++;
    }
    m_cvsGrid->AutoSize();
    this->Layout();
}

void DynaRangeFrame::LoadGraphImage(const wxString& full_path_or_raw_filename)
{
    if (full_path_or_raw_filename.IsEmpty() || !m_imageGraph) {
        return;
    }

    fs::path graphPath;
    std::string displayFilename; // Nombre que se mostrará en la GUI

    // Obtenemos el directorio donde se guardan los resultados
    fs::path output_dir = fs::path(std::string(wxStandardPaths::Get().GetDocumentsDir().mb_str()));
    
    // Verificamos si es una ruta completa o solo el nombre de un RAW
    fs::path input_path(std::string(full_path_or_raw_filename.mb_str()));

    // Si la entrada es la ruta completa del resumen (que viene de m_summaryPlotPath)
    // O si la entrada es un nombre de fichero RAW (como "DSC00001.ARW")
    if (input_path.is_absolute() && fs::exists(input_path)) {
        // Es una ruta absoluta y existente (ej. el summary_plot_path completo)
        graphPath = input_path;
        displayFilename = graphPath.filename().string();
    } else {
        // Asumimos que es un nombre de RAW (ej. "DSC00001.ARW") o un nombre de archivo como "DR_summary_plot.png"
        std::string filename_str = input_path.stem().string(); // "DSC00001"
        std::string plot_filename = filename_str + "_snr_plot.png";
        graphPath = output_dir / plot_filename;
        displayFilename = plot_filename; // Mostrar el nombre del archivo del gráfico
    }

    wxImage image;
    if (!fs::exists(graphPath) || !image.LoadFile(wxString(graphPath.string()))) {
        m_imageGraph->SetBitmap(wxBitmap(1,1));
        m_generateGraphStaticText->SetLabel(_("Generated Graph (Image not found): ") + wxString(displayFilename));
        return;
    }

    m_generateGraphStaticText->SetLabel(_("Generated Graph: ") + wxString(displayFilename));

    wxSize panelSize = m_imageGraph->GetSize();
    if (panelSize.GetWidth() == 0 || panelSize.GetHeight() == 0) return;

    int imgWidth = image.GetWidth();
    int imgHeight = image.GetHeight();

    double hScale = (double)panelSize.GetWidth() / imgWidth;
    double vScale = (double)panelSize.GetHeight() / imgHeight;
    double scale = std::min(hScale, vScale);

    // Solo reescalamos si la imagen es más grande que el panel
    if (scale < 1.0) {
        image.Rescale(imgWidth * scale, imgHeight * scale, wxIMAGE_QUALITY_HIGH);
    }
    
    m_imageGraph->SetBitmap(wxBitmap(image));
    m_resultsPanel->Layout(); // Asegura que el panel se redibuja con la nueva imagen
}