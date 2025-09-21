// File: gui/GuiPresenter.hpp
/**
 * @file GuiPresenter.hpp
 * @brief Declares the Presenter for the main GUI frame.
 * @details This class decouples the application logic from the wxWidgets UI (the View).
 */
#pragma once
#include "../core/arguments/ProgramOptions.hpp"
#include "../core/engine/Reporting.hpp" // For ReportOutput
#include <string>
#include <vector>
#include <thread>
#include <atomic>

// Forward declaration of the View to avoid circular includes
class DynaRangeFrame; 

class GuiPresenter {
public:
    /**
     * @brief Constructs the Presenter.
     * @param view A pointer to the DynaRangeFrame (the View) it will manage.
     */
    explicit GuiPresenter(DynaRangeFrame* view);
    ~GuiPresenter();

    /**
     * @brief Starts the dynamic range analysis in a background thread.
     */
    void StartAnalysis();

    /**
     * @brief Adds a list of files to the current input list.
     * @param files The file paths to add.
     */
    void AddInputFiles(const std::vector<std::string>& files);

    /**
     * @brief Updates the command preview text by querying the View for its current state.
     */
    void UpdateCommandPreview();

    /**
     * @brief Handles a click on the results grid.
     * @param row The row index that was clicked.
     */
    void HandleGridCellClick(int row);

    /**
     * @brief Gets the options used for the most recent analysis run.
     * @return A const reference to the ProgramOptions struct.
     */
    const ProgramOptions& GetLastRunOptions() const;

    /**
     * @brief Gets the report output from the most recent analysis run.
     * @return A const reference to the ReportOutput struct.
     */
    const ReportOutput& GetLastReport() const;

    bool IsWorkerRunning() const;
    void RemoveInputFiles(const std::vector<int>& indices);
    void RequestWorkerCancellation();

private:
    /**
     * @brief The main function for the worker thread.
     * @param opts The configuration to use for the analysis.
     */
    void AnalysisWorker(ProgramOptions opts);

    /**
     * @brief Gathers current settings from the View and merges them with defaults
     * to create the final ProgramOptions for an analysis run.
     * @return A fully configured ProgramOptions object.
     */
    ProgramOptions BuildProgramOptions();

    // Member variables
    DynaRangeFrame* m_view; // Pointer to the View

    // Application State
    std::vector<std::string> m_inputFiles;
    ReportOutput m_lastReport;
    ProgramOptions m_lastRunOptions;
    std::thread m_workerThread;
    std::atomic<bool> m_isWorkerRunning{false};
    std::atomic<bool> m_cancelWorker{false};
};