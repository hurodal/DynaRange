// File: gui/GuiPresenter.cpp
/**
 * @file gui/GuiPresenter.cpp
 * @brief Implements the application logic presenter for the GUI.
 */
#include "GuiPresenter.hpp"
#include "DynaRangeFrame.hpp" // Include the full View definition
#include "helpers/GuiPlotter.hpp" // Include the new GUI plotter
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/utils/CommandGenerator.hpp"
#include <algorithm>
#include <ostream>
#include <set>
#include <streambuf>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <filesystem>

namespace fs = std::filesystem;

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
  mgr.Set("snrthreshold-db", m_view->GetSnrThresholds());
  mgr.Set("drnormalization-mpx", m_view->GetDrNormalization());
  mgr.Set("poly-fit", m_view->GetPolyOrder());
  
  // Set plot-related arguments based on GUI state to prevent crash
  int plotMode = m_view->GetPlotMode();
  mgr.Set("plot", plotMode);
  mgr.Set("generate-plot", plotMode != 0); // Plot mode 0 is "No"
  if (plotMode != 0) {
      mgr.Set("plot-format", m_view->GetPlotFormat());
  }

  mgr.Set("chart-coords", m_view->GetChartCoords());

  // Correctly set the "chart-patches" argument as a vector of two integers.
  std::vector<int> patches = {m_view->GetChartPatchesM(), m_view->GetChartPatchesN()};
  mgr.Set("chart-patches", patches);

  // Read the state of the new channel checkboxes and set the argument.
  RawChannelSelection channels = m_view->GetRawChannelSelection();
  std::vector<int> channels_vec = {
      static_cast<int>(channels.R),
      static_cast<int>(channels.G1),
      static_cast<int>(channels.G2),
      static_cast<int>(channels.B),
      static_cast<int>(channels.AVG)
  };
  mgr.Set("raw-channel", channels_vec);
  
  bool black_is_default = m_view->GetDarkFilePath().empty();
  mgr.Set("black-level-is-default", black_is_default);
  bool sat_is_default = m_view->GetSaturationFilePath().empty();
  mgr.Set("saturation-level-is-default", sat_is_default);
  
  // The default flag is now determined by the user providing the argument in the CLI.
  // In the GUI, we can assume if the user has touched the control, it's not default.
  // A simple check is to see if the content is different from the default "0 12".
  std::vector<double> current_thresholds = m_view->GetSnrThresholds();
  bool snr_is_default = (current_thresholds.size() == 2 && current_thresholds[0] == 0 && current_thresholds[1] == 12);
  mgr.Set("snr-threshold-is-default", snr_is_default);


  // Añadir el valor de print-patches al manager
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
    if (!m_view->ValidateSnrThresholds()) {
        m_view->ShowError(
            _("Invalid Input"),
            _("The 'SNR Thresholds' field contains invalid characters. Please enter only numbers separated by spaces.")
        );
        return; // Abort the analysis
    }

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
  // Clear previous results
  m_summaryImage = wxImage();
  m_individualImages.clear();

  WxLogStreambuf log_streambuf(m_view);
  std::ostream log_stream(&log_streambuf);

  // 1. Run core analysis to get raw data and file-based reports
  m_lastReport =
      DynaRange::RunDynamicRangeAnalysis(opts, log_stream, m_cancelWorker);

  if (m_cancelWorker) {
      if (m_view) m_view->PostAnalysisComplete();
      return;
  }
  
  // 2. After core analysis, generate in-memory images for the GUI
  if (opts.generate_plot && !m_lastReport.curve_data.empty()) {
      log_stream << _("\nGenerating in-memory plots for GUI...") << std::endl;
      
      // Generate summary plot image
      // --- CORRECTION START ---
      // First, build the full title as a wxString
      wxString summary_title_wx = _("SNR Curves - Summary (") + wxString(m_lastReport.curve_data[0].camera_model) + ")";
      // Then, convert the wxString to std::string for the function call
      std::string summary_title = std::string(summary_title_wx.mb_str());
      // --- CORRECTION END ---
      m_summaryImage = GuiPlotter::GeneratePlotAsWxImage(m_lastReport.curve_data, m_lastReport.dr_results, summary_title, opts);

      // Generate individual plot images
      std::map<std::string, std::vector<CurveData>> curves_by_file;
      for(const auto& curve : m_lastReport.curve_data) {
          curves_by_file[curve.filename].push_back(curve);
      }
       std::map<std::string, std::vector<DynamicRangeResult>> results_by_file;
      for(const auto& result : m_lastReport.dr_results) {
          results_by_file[result.filename].push_back(result);
      }

      for(const auto& pair : curves_by_file) {
          const std::string& filename = pair.first;
          std::stringstream title_ss;
          title_ss << fs::path(filename).filename().string() << " (" << pair.second[0].camera_model << ")";
          m_individualImages[filename] = GuiPlotter::GeneratePlotAsWxImage(pair.second, results_by_file[filename], title_ss.str(), opts);
      }
      log_stream << _("In-memory plot generation complete.") << std::endl;
  }

  // 3. Notify the view on the main thread that all work is done
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

void GuiPresenter::HandleGridCellClick(const std::string& basename) {
    // If the basename is empty, it's a header click or invalid row, so show the summary plot.
    if (basename.empty()) {
        if (m_summaryImage.IsOk()) {
            m_view->DisplayImage(m_summaryImage);
        }
        return;
    }

    // Find the corresponding full path by searching through the last report results.
    // The m_individualImages map uses the full path as its key.
    for (const auto& result : m_lastReport.dr_results) {
        if (fs::path(result.filename).filename().string() == basename) {
            // Found the matching full path.
            const std::string& full_path = result.filename;
            
            if (m_individualImages.count(full_path)) {
                const wxImage& individual_image = m_individualImages.at(full_path);
                if (individual_image.IsOk()) {
                    m_view->DisplayImage(individual_image);
                    return; // Found and displayed, so we are done.
                }
            }
        }
    }
    
    // If we get here, it means no matching image was found for the clicked filename.
    // In this case, we simply do nothing and leave the current image displayed.
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

void GuiPresenter::RemoveAllInputFiles() {
    m_inputFiles.clear();
    m_view->UpdateInputFileList(m_inputFiles);
    UpdateCommandPreview();
}

const ProgramOptions &GuiPresenter::GetLastRunOptions() const {
  return m_lastRunOptions;
}

const ReportOutput &GuiPresenter::GetLastReport() const { return m_lastReport; }

bool GuiPresenter::IsWorkerRunning() const { return m_isWorkerRunning; }

void GuiPresenter::RequestWorkerCancellation() { m_cancelWorker = true; }

const wxImage& GuiPresenter::GetLastSummaryImage() const
{
    return m_summaryImage;
}