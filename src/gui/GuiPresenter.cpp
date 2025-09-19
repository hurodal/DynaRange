// File: gui/GuiPresenter.cpp
/**
 * @file GuiPresenter.cpp
 * @brief Implements the application logic presenter for the GUI.
 */
#include "GuiPresenter.hpp"
#include "DynaRangeFrame.hpp" // Include the full View definition
#include "../core/engine/Engine.hpp"
#include "../core/arguments/CommandGenerator.hpp"
#include <wx/stdpaths.h>
#include <filesystem>
#include <ostream>
#include <streambuf>

namespace fs = std::filesystem;

// --- WORKER THREAD LOGGING SETUP ---
// This streambuf redirects std::ostream to the View's log through wxEvents
class WxLogStreambuf : public std::streambuf {
public:
    WxLogStreambuf(DynaRangeFrame* view) : m_view(view) {}
protected:
    virtual int sync() override {
        if (!m_buffer.empty() && m_view) {
            m_view->PostLogUpdate(m_buffer);
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
    DynaRangeFrame* m_view;
    std::string m_buffer;
};


GuiPresenter::GuiPresenter(DynaRangeFrame* view) : m_view(view) {}

GuiPresenter::~GuiPresenter() {
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void GuiPresenter::StartAnalysis() {
    m_lastRunOptions = BuildProgramOptions();
    if (m_lastRunOptions.input_files.empty()) {
        m_view->ShowError("Error", "Please select at least one input RAW file.");
        return;
    }

    m_view->SetUiState(true); // Tell the view to enter "processing" mode

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread(&GuiPresenter::AnalysisWorker, this, m_lastRunOptions);
}

void GuiPresenter::AnalysisWorker(ProgramOptions opts) {
    WxLogStreambuf log_streambuf(m_view);
    std::ostream log_stream(&log_streambuf);

    m_lastReport = DynaRange::RunDynamicRangeAnalysis(opts, log_stream);
    
    // Notify the view on the main thread that the work is done
    if (m_view) {
        m_view->PostAnalysisComplete();
    }
}

ProgramOptions GuiPresenter::BuildProgramOptions() {
    ProgramOptions opts;
    
    // --- Get values from the View ---
    opts.dark_file_path = m_view->GetDarkFilePath();
    opts.sat_file_path = m_view->GetSaturationFilePath();
    opts.dark_value = m_view->GetDarkValue();
    opts.saturation_value = m_view->GetSaturationValue();
    opts.input_files = m_inputFiles;

    // --- Business Logic: Apply defaults for GUI context ---
    opts.snr_thresholds_db = {12.0, 0.0};
    opts.dr_normalization_mpx = 8.0;
    opts.poly_order = 3;
    opts.patch_ratio = m_view->GetPatchRatio();
    opts.plot_mode = 2; // Always generate plots and command for the GUI
    
    wxString docsPath = wxStandardPaths::Get().GetDocumentsDir();
    fs::path output_dir = fs::path(std::string(docsPath.mb_str()));
    opts.output_filename = (output_dir / "DR_results.csv").string();

    return opts;
}

void GuiPresenter::AddInputFiles(const std::vector<std::string>& files) {
    m_inputFiles.insert(m_inputFiles.end(), files.begin(), files.end());
    m_view->UpdateInputFileList(m_inputFiles);
    UpdateCommandPreview();
}

void GuiPresenter::UpdateCommandPreview() {
    ProgramOptions current_opts = BuildProgramOptions();
    std::string command = GenerateCommand(current_opts);
    m_view->UpdateCommandPreview(command);
}

void GuiPresenter::HandleGridCellClick(int row) {
    if (row == 0) { // Summary plot row
        if (m_lastReport.summary_plot_path.has_value()) {
            m_view->LoadGraphImage(*m_lastReport.summary_plot_path);
        }
    } else if (row >= 1) { // Data row for an individual file
        // The grid row index corresponds to the input file index of the last run
        int result_index = row - 1;
        if (result_index < m_lastRunOptions.input_files.size()) {
            std::string filename = m_lastRunOptions.input_files[result_index];
            if (m_lastReport.individual_plot_paths.count(filename)) {
                m_view->LoadGraphImage(m_lastReport.individual_plot_paths.at(filename));
            }
        }
    }
}

const ProgramOptions& GuiPresenter::GetLastRunOptions() const {
    return m_lastRunOptions;
}

const ReportOutput& GuiPresenter::GetLastReport() const {
    return m_lastReport;
}