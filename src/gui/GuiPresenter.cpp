// File: gui/GuiPresenter.cpp
/**
 * @file GuiPresenter.cpp
 * @brief Implements the application logic presenter for the GUI.
 */
#include "GuiPresenter.hpp"
#include "DynaRangeFrame.hpp" // Include the full View definition
#include "../core/engine/Engine.hpp"
#include "../core/arguments/ArgumentManager.hpp"
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>
#include <ostream>
#include <streambuf>
#include <algorithm> // Needed for std::sort


// --- WORKER THREAD LOGGING SETUP ---
// This streambuf redirects std::ostream to the View's log through wxEvents
class WxLogStreambuf : public std::streambuf {
public:
    WxLogStreambuf(DynaRangeFrame* view) : m_view(view) {
        // The line "m_cancelWorker = false;" was here and has been removed.
    }
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


GuiPresenter::GuiPresenter(DynaRangeFrame* view) : m_view(view) {
    m_cancelWorker = false;
}

GuiPresenter::~GuiPresenter() {
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void GuiPresenter::UpdateManagerFromView() {
    auto& mgr = ArgumentManager::Instance();

    // Aquí es donde se conectan los controles
    mgr.Set("black-file", m_view->GetDarkFilePath());
    mgr.Set("saturation-file", m_view->GetSaturationFilePath());
    mgr.Set("black-level", m_view->GetDarkValue());
    mgr.Set("saturation-level", m_view->GetSaturationValue());
    mgr.Set("patch-ratio", m_view->GetPatchRatio());
    mgr.Set("input-files", m_inputFiles); // Usa la lista interna del presenter    
    mgr.Set("output-file", m_view->GetOutputFilePath());
    mgr.Set("snrthreshold-db", m_view->GetSnrThreshold());
    mgr.Set("drnormalization-mpx", m_view->GetDrNormalization());
    mgr.Set("poly-fit", m_view->GetPolyOrder());
    mgr.Set("plot", m_view->GetPlotMode());
}


void GuiPresenter::StartAnalysis() {
    // 1. Actualiza el gestor con los valores actuales de la GUI.
    UpdateManagerFromView();

    // 2. Obtiene las opciones para el motor desde el gestor.
    m_lastRunOptions = ArgumentManager::Instance().ToProgramOptions();
    
    if (m_lastRunOptions.input_files.empty()) {
        m_view->ShowError("Error", "Please select at least one input RAW file.");
        return;
    }

    m_view->SetUiState(true);

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    
    m_cancelWorker = false;
    m_workerThread = std::thread([this] {
        this->AnalysisWorker(m_lastRunOptions);    
        m_isWorkerRunning = false;
    });
}

void GuiPresenter::AnalysisWorker(ProgramOptions opts) {
    m_isWorkerRunning = true; // Inform that the thread has started
    WxLogStreambuf log_streambuf(m_view);
    std::ostream log_stream(&log_streambuf);

    m_lastReport = DynaRange::RunDynamicRangeAnalysis(opts, log_stream, m_cancelWorker);

    // Notify the view on the main thread that the work is done
    if (m_view) {
        m_view->PostAnalysisComplete();
    }
}

void GuiPresenter::AddInputFiles(const std::vector<std::string>& files) {
    m_inputFiles.insert(m_inputFiles.end(), files.begin(), files.end());
    m_view->UpdateInputFileList(m_inputFiles);
    UpdateCommandPreview();
}

void GuiPresenter::UpdateCommandPreview() {
    // Actualiza el gestor con el estado de la GUI y luego genera el comando.
    UpdateManagerFromView();
    std::string command = ArgumentManager::Instance().GenerateCommand();
    m_view->UpdateCommandPreview(command);
}

void GuiPresenter::HandleGridCellClick(int row)
{
    // The header is in row 0. Any click on it should show the summary plot.
    if (row < 1) {
        if (m_lastReport.summary_plot_path.has_value()) {
            m_view->LoadGraphImage(*m_lastReport.summary_plot_path);
        }
    } else { // Data rows start from index 1
        // The result index is the grid row minus 1 (to account for the header row)
        int result_index = row - 1;
        if (result_index < m_lastRunOptions.input_files.size()) {
            std::string filename = m_lastRunOptions.input_files[result_index];
            if (m_lastReport.individual_plot_paths.count(filename)) {
                m_view->LoadGraphImage(m_lastReport.individual_plot_paths.at(filename));
            }
        }
    }
}

void GuiPresenter::RemoveInputFiles(const std::vector<int>& indices) {
    // It is CRITICAL to delete items from the end to the beginning
    // to avoid invalidating the indices of the remaining items.
    std::vector<int> sorted_indices = indices;
    std::sort(sorted_indices.rbegin(), sorted_indices.rend());

    for (int index : sorted_indices) {
        if (index < m_inputFiles.size()) {
            m_inputFiles.erase(m_inputFiles.begin() + index);
        }
    }

    // Notify the view to update itself
    m_view->UpdateInputFileList(m_inputFiles);

    // Call the presenter's own method
    UpdateCommandPreview();
}

const ProgramOptions& GuiPresenter::GetLastRunOptions() const {
    return m_lastRunOptions;
}

const ReportOutput& GuiPresenter::GetLastReport() const {
    return m_lastReport;
}

bool GuiPresenter::IsWorkerRunning() const {
    return m_isWorkerRunning;
}

void GuiPresenter::RequestWorkerCancellation() {
    m_cancelWorker = true;
}