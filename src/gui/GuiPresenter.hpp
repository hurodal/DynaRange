// File: gui/GuiPresenter.hpp
/**
 * @file GuiPresenter.hpp
 * @brief Declares the Presenter for the main GUI frame.
 * @details This class decouples the application logic from the wxWidgets UI
 * (the View).
 */
#pragma once
#include "../core/arguments/ArgumentsOptions.hpp"
#include "../core/engine/Reporting.hpp" // For ReportOutput
#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <wx/image.h>

// Forward declaration of the View to avoid circular includes
class DynaRangeFrame;

class GuiPresenter {
public:
  /**
   * @brief Constructs the Presenter.
   * @param view A pointer to the DynaRangeFrame (the View) it will manage.
   */
  explicit GuiPresenter(DynaRangeFrame *view);
  ~GuiPresenter();

  /**
   * @brief Starts the dynamic range analysis in a background thread.
   */
  void StartAnalysis();

  /**
   * @brief Adds a list of files to the current input list.
   * @param files The file paths to add.
   */
  void AddInputFiles(const std::vector<std::string> &files);

  /**
   * @brief Updates the command preview text by querying the View for its
   * current state.
   */
  void UpdateCommandPreview();

  /**
   * @brief Handles a click on the results grid, triggering the summary plot display.
   */
  void HandleGridCellClick();
  
  /**
   * @brief Gets the options used for the most recent analysis run.
   * @return A const reference to the ProgramOptions struct.
   */
  const ProgramOptions &GetLastRunOptions() const;

  /**
   * @brief Gets the report output from the most recent analysis run.
   * @return A const reference to the ReportOutput struct.
   */
  const ReportOutput &GetLastReport() const;
  
  /**
   * @brief Gets the last generated summary plot image.
   * @return A const reference to the wxImage.
   */
  const wxImage& GetLastSummaryImage() const;

  /**
   * @brief Checks if the worker thread is currently running.
   * @return true if the worker is active, false otherwise.
   */
  bool IsWorkerRunning() const;

  /**
   * @brief Removes a list of files from the input list by their indices.
   * @param indices The vector of zero-based indices to remove.
   */
  void RemoveInputFiles(const std::vector<int> &indices);

  /**
   * @brief Removes all files from the current input list.
   */
  void RemoveAllInputFiles();

  /**
   * @brief Signals the worker thread to stop its processing.
   */
  void RequestWorkerCancellation();

  /**
   * @brief Updates the input file list by filtering out selected calibration files.
   * @details This method ensures that if a user selects a dark or saturation
   * file that is already in the input list, it is removed from that list to
   * avoid being processed as both a calibration and an analysis file.
   */
  void UpdateCalibrationFiles();

private:
  /**
   * @brief The main function for the worker thread.
   * @param opts The configuration to use for the analysis.
   */
  void AnalysisWorker(ProgramOptions opts);

  /**
   * @brief Gathers current settings from the View and updates the
   * ArgumentManager.
   * @details This is the bridge that synchronizes the GUI state with the
   * centralized argument system before an analysis or command preview
   * generation.
   */
  void UpdateManagerFromView();

  // Member variables
  DynaRangeFrame *m_view; // Pointer to the View

  // Application State
  std::vector<std::string> m_inputFiles;
  ReportOutput m_lastReport;
  ProgramOptions m_lastRunOptions;
  
  // In-memory images for the GUI
  wxImage m_summaryImage;
  std::map<std::string, wxImage> m_individualImages;

  std::thread m_workerThread;
  std::atomic<bool> m_isWorkerRunning{false};
  std::atomic<bool> m_cancelWorker{false};
};