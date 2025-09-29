// File: gui/GuiPresenter.cpp
/**
 * @file gui/GuiPresenter.cpp
 * @brief Implements the application logic presenter for the GUI.
 */
#include "GuiPresenter.hpp"
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/engine/Engine.hpp"
#include "DynaRangeFrame.hpp" // Include the full View definition
#include "../core/utils/CommandGenerator.hpp"
#include <algorithm>          // Needed for std::sort
#include <ostream>
#include <set>
#include <streambuf>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

// --- WORKER THREAD LOGGING SETUP ---
// This streambuf redirects std::ostream to the View's log through wxEvents
class WxLogStreambuf : public std::streambuf {
public:
  WxLogStreambuf(DynaRangeFrame *view) : m_view(view) {}

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
      if (c == '\n') {
        sync();
      }
    }
    return c;
  }

private:
  DynaRangeFrame *m_view;
  std::string m_buffer;
};

GuiPresenter::GuiPresenter(DynaRangeFrame *view) : m_view(view) {
  m_cancelWorker = false;
}

GuiPresenter::~GuiPresenter() {
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }
}

void GuiPresenter::UpdateManagerFromView() {
  auto &mgr = ArgumentManager::Instance();
  // This is where the controls are connected to the argument manager
  mgr.Set("black-file", m_view->GetDarkFilePath());
  mgr.Set("saturation-file", m_view->GetSaturationFilePath());
  mgr.Set("black-level", m_view->GetDarkValue());
  mgr.Set("saturation-level", m_view->GetSaturationValue());
  mgr.Set("patch-ratio", m_view->GetPatchRatio());
  mgr.Set("input-files", m_view->GetInputFiles());
  mgr.Set("output-file", m_view->GetOutputFilePath());
  mgr.Set("snrthreshold-db", m_view->GetSnrThreshold());
  mgr.Set("drnormalization-mpx", m_view->GetDrNormalization());
  mgr.Set("poly-fit", m_view->GetPolyOrder());
  mgr.Set("plot", m_view->GetPlotMode());
  mgr.Set("chart-coords", m_view->GetChartCoords());

  // Correctly set the "chart-patches" argument as a vector of two integers.
  std::vector<int> patches = {m_view->GetChartPatchesM(), m_view->GetChartPatchesN()};
  mgr.Set("chart-patches", patches);
  mgr.Set("print-patches", m_view->GetPrintPatchesFilename());
}

void GuiPresenter::UpdateCommandPreview() {

  // First, sync the manager with the current state of the GUI controls
  UpdateManagerFromView();

  // The call to generate the command is now delegated to the new CommandGenerator module.
  std::string command = CommandGenerator::GenerateCommand(CommandFormat::GuiPreview);
  
  // Update the view with the newly generated command
  m_view->UpdateCommandPreview(command);
}

void GuiPresenter::StartAnalysis() {

  // 1. Update the manager with the current values from the GUI.
  UpdateManagerFromView();

  // 2. Get the options for the engine from the manager.
  m_lastRunOptions = ArgumentManager::Instance().ToProgramOptions();
  if (m_lastRunOptions.input_files.empty()) {
    m_view->ShowError(_("Error"),
                      _("Please select at least one input RAW file."));
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
  m_isWorkerRunning = true;
  // Inform that the thread has started
  WxLogStreambuf log_streambuf(m_view);
  std::ostream log_stream(&log_streambuf);

  m_lastReport =
      DynaRange::RunDynamicRangeAnalysis(opts, log_stream, m_cancelWorker);
  // Notify the view on the main thread that the work is done
  if (m_view) {
    m_view->PostAnalysisComplete();
  }
}

void GuiPresenter::AddInputFiles(const std::vector<std::string> &files_to_add) {
  std::vector<std::string> current_files = m_view->GetInputFiles();

  // Use a set to efficiently track existing files and prevent duplicates.
  std::set<std::string> existing_files(current_files.begin(),
                                       current_files.end());

  for (const auto &file : files_to_add) {
    // The insert method of a set returns a pair. The '.second' element is
    // true if the insertion took place (the element was new), and false if it
    // was already there.
    if (existing_files.insert(file).second) {
      // If the file was new, add it to our list to be displayed.
      current_files.push_back(file);
    }
  }

  // Update the view with the final, de-duplicated list.
  m_view->UpdateInputFileList(current_files);
  UpdateCommandPreview();
}

void GuiPresenter::HandleGridCellClick(int row) {
    // A click on the column header labels (row < 0) or the first data row
    // (row == 0), which also contains the headers, displays the summary plot.
    if (row <= 0) {
        if (m_lastReport.summary_plot_path.has_value()) {
            m_view->LoadGraphImage(*m_lastReport.summary_plot_path);
        }
    } else { // Data rows (e.g., iso00200.dng) start from grid row index 1.
        // The result index in our data vectors corresponds to the grid row minus 1.
        int result_index = row - 1;
        
        // Use the sorted results list (m_lastReport.dr_results) as the
        // source of truth for the filename. This list is guaranteed to be in the
        // same order as the grid view.
        if (result_index < m_lastReport.dr_results.size()) {
            // Get the filename directly from the sorted results list.
            std::string filename = m_lastReport.dr_results[result_index].filename;
            
            if (m_lastReport.individual_plot_paths.count(filename)) {
                m_view->LoadGraphImage(m_lastReport.individual_plot_paths.at(filename));
            }
        }
    }
}

void GuiPresenter::RemoveInputFiles(const std::vector<int> &indices) {
  // It is CRITICAL to delete items from the end to the beginning
  // to avoid invalidating the indices of the remaining items.
  std::vector<std::string> current_files = m_view->GetInputFiles();
  std::vector<int> sorted_indices = indices;
  std::sort(sorted_indices.rbegin(), sorted_indices.rend());

  for (int index : sorted_indices) {
    if (index < current_files.size()) {
      current_files.erase(current_files.begin() + index);
    }
  }

  // Notify the view to update itself
  m_view->UpdateInputFileList(current_files);
  // Call the presenter's own method
  UpdateCommandPreview();
}

const ProgramOptions &GuiPresenter::GetLastRunOptions() const {
  return m_lastRunOptions;
}

const ReportOutput &GuiPresenter::GetLastReport() const { return m_lastReport; }

bool GuiPresenter::IsWorkerRunning() const { return m_isWorkerRunning; }

void GuiPresenter::RequestWorkerCancellation() { m_cancelWorker = true; }