/**
 * @file DynaRangeFrame.cpp
 * @brief Implementation of the DynaRange GUI's main frame.
 * @author Juanma Font
 * @date 2025-09-10
 */
// Fichero: gui/DynaRangeFrame.cpp
#include "DynaRangeFrame.hpp"
#include "../core/Engine.hpp"
#include "../core/Analysis.hpp" 
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


/**
 * @brief Called by wxWidgets when files are dropped onto the associated window.
 * @param x The x-coordinate of the drop point.
 * @param y The y-coordinate of the drop point.
 * @param filenames An array of full paths for the dropped files.
 * @return true to indicate that the drop was successfully handled.
 */
bool FileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    if (m_owner) {
        m_owner->AddDroppedFiles(filenames);
    }
    return true;
}

// --- MAIN FRAME IMPLEMENTATION ---
/**
 * @brief Constructor for the main application frame.
 * @param parent Parent window, usually NULL for the main frame.
 */
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
    // --- Drag and Drop Initialization ---
    m_dropTarget = new FileDropTarget(this);
    SetDropTarget(m_dropTarget); // Enable the entire frame to accept dropped files.

    UpdateCommandPreview();

    // Cargar el logotipo al iniciar la aplicación.
    // Busca el logo en el mismo directorio que el ejecutable.
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fn(exePath);
    wxString appDir = fn.GetPath();
    wxString logoPath = appDir + wxFILE_SEP_PATH + "logo.png";
    
    wxImage logoImage;
    if (logoImage.LoadFile(logoPath, wxBITMAP_TYPE_PNG)) {
        m_imageGraph->SetBitmap(wxBitmap(logoImage));
        m_generateGraphStaticText->SetLabel(_("Welcome to Dynamic Range Calculator"));
    } else {
        m_imageGraph->SetBitmap(wxBitmap()); // Si no hay logo, usa un bitmap nulo
        m_generateGraphStaticText->SetLabel(_("Welcome (logo.png not found)"));
    }
    m_resultsPanel->Layout();
}

/**
 * @brief Destructor for the main application frame.
 */
DynaRangeFrame::~DynaRangeFrame()
{
    // Unbind events to prevent issues on closing
    m_executeButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnExecuteClick, this);
    m_addRawFilesButton->Unbind(wxEVT_BUTTON, &DynaRangeFrame::OnAddFilesClick, this);
    m_cvsGrid->Unbind(wxEVT_GRID_CELL_LEFT_CLICK, &DynaRangeFrame::OnGridCellClick, this);
    Unbind(wxEVT_COMMAND_WORKER_UPDATE, &DynaRangeFrame::OnWorkerUpdate, this);
    Unbind(wxEVT_COMMAND_WORKER_COMPLETED, &DynaRangeFrame::OnWorkerCompleted, this);
    m_darkFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_saturationFilePicker->Unbind(wxEVT_FILEPICKER_CHANGED, &DynaRangeFrame::OnInputChanged, this);
    m_darkValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);
    m_saturationValueTextCtrl->Unbind(wxEVT_TEXT, &DynaRangeFrame::OnInputChanged, this);

    // The SetDropTarget call transfers ownership, so we don't delete m_dropTarget here.
    // Setting it to null is good practice.
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

    // Deshabilitamos el panel y mostramos el logo como placeholder.
    m_resultsPanel->Enable(false); 

    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fn(exePath);
    wxString appDir = fn.GetPath();
    wxString logoPath = appDir + wxFILE_SEP_PATH + "logo.png";
    
    wxImage logoImage;
    if (logoImage.LoadFile(logoPath, wxBITMAP_TYPE_PNG)) {
        m_imageGraph->SetBitmap(wxBitmap(logoImage));
        m_generateGraphStaticText->SetLabel(_("Processing... Please wait."));
    } else {
        m_imageGraph->SetBitmap(wxBitmap());
        m_generateGraphStaticText->SetLabel(_("Processing..."));
    }
    m_resultsPanel->Layout();

    // Limpiamos el grid antes de empezar
    if (m_cvsGrid->GetNumberRows() > 0) m_cvsGrid->DeleteRows(0, m_cvsGrid->GetNumberRows());
    if (m_cvsGrid->GetNumberCols() > 0) m_cvsGrid->DeleteCols(0, m_cvsGrid->GetNumberCols());

    std::thread worker_thread([this]() {
        WxLogStreambuf log_streambuf(this);
        std::ostream log_stream(&log_streambuf);
        
        ProgramOptions opts = m_lastRunOptions;
        m_summaryPlotPath.clear();

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
    m_resultsPanel->Enable(true); // Habilitamos el panel de nuevo
    
    if (m_summaryPlotPath.empty()) {
        AppendLog(_("\n---\nExecution failed. Please check the log for details."));
        wxMessageBox(_("An error occurred during processing. Please check the log tab for details."), _("Error"), wxOK | wxICON_ERROR, this);
        m_generateGraphStaticText->SetLabel(_("Error during processing. Check log."));
    } else {
        AppendLog(_("\n---\nExecution finished successfully."));
        m_mainNotebook->SetSelection(2);
        LoadResults(m_lastRunOptions);
        LoadGraphImage(wxString(m_summaryPlotPath));
    }
}

void DynaRangeFrame::OnGridCellClick(wxGridEvent& event)
{
    int row = event.GetRow();

    // Fila -1: Cabeceras grises (A, B, C...)
    // Fila 0: Títulos de texto ("raw_file", etc.)
    // Fila 1 en adelante: Datos de los RAWs
    if (row < 1) { 
        if (!m_summaryPlotPath.empty()) {
            LoadGraphImage(wxString(m_summaryPlotPath));
        }
    } else { 
        // **CORRECCIÓN DE ÍNDICE**:
        // La fila 1 del grid es la primera de datos, que corresponde al raw en la fila 1 del grid
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
    
    // Set GUI defaults according to new argument specifications
    opts.snr_thresholds_db = {12.0, 0.0}; // Default behavior: calculate for 12dB and 0dB
    opts.dr_normalization_mpx = 8.0;
    opts.poly_order = DEFAULT_POLY_ORDER;
    opts.patch_ratio = 0.5; // New argument, using default value
    opts.plot_mode = 2;     // The GUI will always generate the plot with the command
    
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
    std::string command_string = GenerateCommandString(opts);
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

void DynaRangeFrame::LoadResults(const ProgramOptions& opts) {
    std::ifstream file(opts.output_filename);
    if (!file) { return; }
    
    // Limpieza del grid por si acaso
    if (m_cvsGrid->GetNumberRows() > 0) m_cvsGrid->DeleteRows(0, m_cvsGrid->GetNumberRows());
    if (m_cvsGrid->GetNumberCols() > 0) m_cvsGrid->DeleteCols(0, m_cvsGrid->GetNumberCols());

    std::string line;
    int csv_row_index = 0;
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        int col = 0;

        if (csv_row_index == 0) { // Cabecera del CSV
            // Añadimos la fila para los títulos de texto
            m_cvsGrid->AppendRows(1);

            while (std::getline(ss, cell, ',')) {
                if (col >= m_cvsGrid->GetNumberCols()) m_cvsGrid->AppendCols(1);
                // Ponemos los títulos en la Fila 0 del grid
                m_cvsGrid->SetCellValue(0, col, cell); 
                col++;
            }
        } else { // Datos del CSV
            m_cvsGrid->AppendRows(1);
            // La primera línea de datos del CSV (csv_row_index=1) va en la Fila 1 del grid
            int grid_row = csv_row_index; 
            while (std::getline(ss, cell, ',')) {
                if (col >= m_cvsGrid->GetNumberCols()) m_cvsGrid->AppendCols(1);
                m_cvsGrid->SetCellValue(grid_row, col, cell);
                col++;
            }
        }
        csv_row_index++;
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

/**
 * @brief Checks if a given file is a RAW format supported by LibRaw.
 *
 * This function attempts to open the file with LibRaw to confirm it is a
 * valid and supported RAW format, without fully decoding it.
 * @param filePath The full path to the file.
 * @return true if the file is a supported RAW, false otherwise.
 */
bool DynaRangeFrame::IsSupportedRawFile(const wxString& filePath)
{
    LibRaw raw_processor;
    // Use mb_str() to convert wxString to a standard C-string for LibRaw
    if (raw_processor.open_file(filePath.mb_str()) == LIBRAW_SUCCESS) {
        // Success! LibRaw can open it.
        return true;
    }
    return false;
}

/**
 * @brief Filters a list of file paths, adding only valid RAWs to the input list.
 *
 * This function iterates through a given list of file paths, checks each one
 * to see if it's a supported RAW file, and updates the UI. It also informs
 * the user if any files were rejected.
 * @param paths An array of file paths to process.
 */
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

/**
 * @brief Handles the 'Add RAW Files...' button click event.
 *
 * Opens a file dialog to allow the user to select multiple RAW files, then
 * filters and adds them to the list.
 * @param event The command event.
 */
void DynaRangeFrame::OnAddFilesClick(wxCommandEvent& event) 
{
    wxFileDialog openFileDialog(this, _("Select RAW files"), "", "", "RAW files (*.dng;*.cr2;*.nef;*.orf;*.arw)|*.dng;*.cr2;*.nef;*.orf;*.arw|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return;
    }
    
    wxArrayString paths;
    openFileDialog.GetPaths(paths);
    AddRawFilesToList(paths); // Use the centralized function
}


/**
 * @brief Adds a list of files, received from a drop operation, to the input list.
 *
 * This function is called by the FileDropTarget class. It filters and adds
 * the dropped files to the list.
 * @param filenames An array of full file paths.
 */
void DynaRangeFrame::AddDroppedFiles(const wxArrayString& filenames)
{
    AddRawFilesToList(filenames); // Use the centralized function
}