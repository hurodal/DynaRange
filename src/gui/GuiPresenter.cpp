// File: gui/GuiPresenter.cpp
/**
 * @file gui/GuiPresenter.cpp
 * @brief Implements the application logic presenter for the GUI.
 */
#include "GuiPresenter.hpp"
#include "DynaRangeFrame.hpp"
#include "controllers/InputController.hpp"
#include "helpers/GuiPlotter.hpp"
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/utils/CommandGenerator.hpp"
#include "../core/utils/OutputNamingContext.hpp"
#include "../core/arguments/Constants.hpp"
#include "../core/setup/PreAnalysis.hpp"
#include <algorithm>
#include <future>
#include <ostream>
#include <set>
#include <streambuf>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/busyinfo.h>
#include <wx/app.h>
#include <thread>

// --- WORKER THREAD LOGGING SETUP ---
// This streambuf redirects std::ostream to the View's log through wxEvents
class WxLogStreambuf : public std::streambuf {
public:
    WxLogStreambuf(DynaRangeFrame* view)
        : m_view(view)
    {
    }

protected:
    virtual int sync() override
    {
        if (!m_buffer.empty() && m_view) {
            m_view->PostLogUpdate(m_buffer);
            m_buffer.clear();
        }
        return 0;
    }

    virtual int overflow(int c) override
    {
        if (c != EOF) {
            m_buffer += static_cast<char>(c);
            if (c == '\n') {
                sync();
            }
        }
        return c;
    }

private:
    DynaRangeFrame* m_view;
    std::string m_buffer;
};

GuiPresenter::GuiPresenter(DynaRangeFrame* view)
    : m_view(view)
{
    m_cancelWorker = false;
}

GuiPresenter::~GuiPresenter()
{
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void GuiPresenter::UpdateManagerFromView()
{
    using namespace DynaRange::Arguments::Constants;
    auto& mgr = ArgumentManager::Instance();
    // Check if view and input controller are valid
    if (!m_view || !m_view->GetInputController()) {
        return;
    }
    const InputController* inputCtrl = m_view->GetInputController();

    // --- Update ArgumentManager with values from GUI controls (most are unchanged) ---
    mgr.Set(InputFiles, m_inputFileManager.GetInputFiles()); // Use manager's clean list
    mgr.Set(BlackFile, inputCtrl->GetDarkFilePath());
    mgr.Set(SaturationFile, inputCtrl->GetSaturationFilePath());
    mgr.Set(BlackLevel, inputCtrl->GetDarkValue());
    mgr.Set(SaturationLevel, inputCtrl->GetSaturationValue());
    mgr.Set(PatchRatio, inputCtrl->GetPatchRatio());
    mgr.Set(OutputFile, inputCtrl->GetOutputFilePath());
    mgr.Set(PrintPatches, inputCtrl->GetPrintPatchesFilename());
    mgr.Set(SnrThresholdDb, inputCtrl->GetSnrThresholds());
    mgr.Set(DrNormalizationMpx, inputCtrl->GetDrNormalization());
    mgr.Set(PolyFit, inputCtrl->GetPolyOrder());
    mgr.Set(ChartCoords, inputCtrl->GetChartCoords());
    std::vector<int> patches = { inputCtrl->GetChartPatchesM(), inputCtrl->GetChartPatchesN() };
    mgr.Set(ChartPatches, patches);
    // Plotting flags and parameters
    int plotModeChoice = inputCtrl->GetPlotMode();
    bool generatePlot = (plotModeChoice != 0);
    mgr.Set(GeneratePlot, generatePlot);
    if (generatePlot) {
        // Plot Format
        DynaRange::Graphics::Constants::PlotOutputFormat format_enum = inputCtrl->GetPlotFormat();
        std::string format_str = "PNG";
        if (format_enum == DynaRange::Graphics::Constants::PlotOutputFormat::PDF) format_str = "PDF";
        else if (format_enum == DynaRange::Graphics::Constants::PlotOutputFormat::SVG) format_str = "SVG";
        mgr.Set(PlotFormat, format_str);
        // Plot Details & Command Mode
        PlottingDetails details = inputCtrl->GetPlottingDetails();
        std::vector<int> params = { static_cast<int>(details.show_scatters), static_cast<int>(details.show_curve), static_cast<int>(details.show_labels), plotModeChoice };
        mgr.Set(PlotParams, params);
    }

    // Channel Selection
    RawChannelSelection channels = inputCtrl->GetRawChannelSelection();
    std::vector<int> channels_vec = { static_cast<int>(channels.R), static_cast<int>(channels.G1), static_cast<int>(channels.G2), static_cast<int>(channels.B), static_cast<int>(channels.avg_mode) };
    mgr.Set(RawChannels, channels_vec);
    // Internal flags (Calibration Defaults, SNR Default)
    mgr.Set(BlackLevelIsDefault, inputCtrl->ShouldEstimateBlackLevel());
    mgr.Set(SaturationLevelIsDefault, inputCtrl->ShouldEstimateSaturationLevel());
    std::vector<double> current_thresholds = inputCtrl->GetSnrThresholds();
    const std::vector<double> default_snr = DEFAULT_SNR_THRESHOLDS_DB;
    mgr.Set(SnrThresholdIsDefault, current_thresholds == default_snr);
    mgr.Set(FullDebug, inputCtrl->ShouldGenerateFullDebug()); // Leer estado del checkbox
    mgr.Set(GuiManualCameraName, inputCtrl->GetManualCameraName());
    mgr.Set(GuiUseExifNameFlag, inputCtrl->GetUseExifNameFlag());
    mgr.Set(GuiUseSuffixFlag, inputCtrl->GetUseSuffixFlag());
}

/**
 * @brief The main function for the background analysis worker thread.
 * @details Runs the core analysis engine, generates in-memory plots for the GUI,
 * and posts completion events back to the main thread.
 * @param opts A copy of the ProgramOptions struct containing all settings gathered from the GUI.
 */
void GuiPresenter::AnalysisWorker(ProgramOptions opts)
{
    m_isWorkerRunning = true; // Set running flag
    // Clear previous results from memory
    m_summaryImage = wxImage();
    m_individualImages.clear();

    // Setup stream buffer to redirect std::cout/cerr to GUI log panel
    WxLogStreambuf log_streambuf(m_view);
    std::ostream log_stream(&log_streambuf);

    // 1. Run the core dynamic range analysis engine
    m_lastReport = DynaRange::RunDynamicRangeAnalysis(opts, log_stream, m_cancelWorker);

    // Check for cancellation
    if (m_cancelWorker) {
        if (m_view) m_view->PostAnalysisComplete();
        // m_isWorkerRunning = false; // Set by caller thread after join
        return;
    }
    // Check for early failure
     if (m_lastReport.final_csv_path.empty() && opts.generate_plot && !m_lastReport.curve_data.empty()) {
          log_stream << _("\nWarning: Analysis completed but failed to save CSV. Plot generation might proceed.") << std::endl;
     } else if (m_lastReport.final_csv_path.empty()) {
          if (m_view) m_view->PostAnalysisComplete();
          // m_isWorkerRunning = false; // Set by caller thread after join
          return;
     }

    // 2. Generate in-memory plot images if requested and possible
    if (opts.generate_plot && !m_lastReport.curve_data.empty()) {
        log_stream << _("\nGenerating in-memory plots for GUI...") << std::endl;

        // Create ReportingParameters
        ReportingParameters reporting_params { /* ... */
            .raw_channels = opts.raw_channels,
            .generate_plot = opts.generate_plot,
            .plot_format = opts.plot_format,
            .plot_details = opts.plot_details,
            .plot_command_mode = opts.plot_command_mode,
            .generated_command = m_lastReport.curve_data[0].generated_command,
            .dark_value = opts.dark_value,
            .saturation_value = opts.saturation_value,
            .black_level_is_default = opts.black_level_is_default,
            .saturation_level_is_default = opts.saturation_level_is_default,
            .snr_thresholds_db = opts.snr_thresholds_db
        };

        // Create OutputNamingContext
        OutputNamingContext naming_ctx;
        naming_ctx.camera_name_exif = m_lastReport.curve_data[0].camera_model;
        naming_ctx.raw_channels = opts.raw_channels;
        naming_ctx.plot_format = opts.plot_format;

        // --- Determine Effective Camera Name based on opts (GUI state copy) ---
        std::string effective_name = "";
        if (opts.gui_use_camera_suffix) {
            if (opts.gui_use_exif_camera_name) {
                effective_name = naming_ctx.camera_name_exif;
            } else {
                effective_name = opts.gui_manual_camera_name;
            }
        }
        naming_ctx.effective_camera_name_for_output = effective_name;

        // *** NUEVO: Log de depuración del estado recibido y el nombre efectivo ***
        wxLogDebug("GuiPresenter::AnalysisWorker - Received Options State:");
        wxLogDebug("  gui_use_camera_suffix: %s", opts.gui_use_camera_suffix ? "true" : "false");
        wxLogDebug("  gui_use_exif_camera_name: %s", opts.gui_use_exif_camera_name ? "true" : "false");
        wxLogDebug("  gui_manual_camera_name: '%s'", opts.gui_manual_camera_name);
        wxLogDebug("  => effective_camera_name_for_output: '%s'", naming_ctx.effective_camera_name_for_output);
        // *** FIN NUEVO ***


        // Generate summary plot image
        m_summaryImage = GuiPlotter::GeneratePlotAsWxImage(m_lastReport.curve_data, m_lastReport.dr_results, naming_ctx, reporting_params);

        // Generate individual plot images if requested
        if (opts.generate_individual_plots) {
             std::map<std::string, std::vector<CurveData>> curves_by_file;
             for (const auto& curve : m_lastReport.curve_data) curves_by_file[curve.filename].push_back(curve);
             std::map<std::string, std::vector<DynamicRangeResult>> results_by_file;
             for (const auto& result : m_lastReport.dr_results) results_by_file[result.filename].push_back(result);

             for (const auto& pair : curves_by_file) {
                 const std::string& filename = pair.first;
                 if (results_by_file.find(filename) == results_by_file.end()) continue;
                 OutputNamingContext individual_ctx = naming_ctx; // Copy context
                 if (!pair.second.empty()) individual_ctx.iso_speed = pair.second[0].iso_speed;
                 else individual_ctx.iso_speed = std::nullopt;
                 m_individualImages[filename] = GuiPlotter::GeneratePlotAsWxImage(pair.second, results_by_file.at(filename), individual_ctx, reporting_params);
             }
        }
        log_stream << _("In-memory plot generation complete.") << std::endl;
    }

    // 3. Notify main thread of completion
    if (m_view) {
        m_view->PostAnalysisComplete();
    }
}

void GuiPresenter::UpdateCommandPreview()
{

    // First, sync the manager with the current state of the GUI controls
    UpdateManagerFromView();

    // The call to generate the command is now delegated to the new CommandGenerator module.
    std::string command = CommandGenerator::GenerateCommand(CommandFormat::GuiPreview);

    // Update the view with the newly generated command
    m_view->UpdateCommandPreview(command);
}

void GuiPresenter::StartAnalysis()
{
    // 1. Validate critical inputs (e.g., SNR thresholds format)
    if (!m_view || !m_view->ValidateSnrThresholds()) {
        if(m_view) m_view->ShowError(_("Invalid Input"), _("The 'SNR Thresholds' field contains invalid characters. Please enter only numbers separated by spaces."));
        return;
    }

    // 2. Synchronize ArgumentManager with the current GUI state
    UpdateManagerFromView();

    // 3. Create the ProgramOptions struct for this specific run
    // This reads all settings, including GUI flags, from ArgumentManager
    m_lastRunOptions = ArgumentManager::Instance().ToProgramOptions();

    // *** NUEVO: Log de depuración del estado capturado ***
    wxLogDebug("GuiPresenter::StartAnalysis - Captured Options State:");
    wxLogDebug("  gui_use_camera_suffix: %s", m_lastRunOptions.gui_use_camera_suffix ? "true" : "false");
    wxLogDebug("  gui_use_exif_camera_name: %s", m_lastRunOptions.gui_use_exif_camera_name ? "true" : "false");
    wxLogDebug("  gui_manual_camera_name: '%s'", m_lastRunOptions.gui_manual_camera_name);
    // *** FIN NUEVO ***

    // 4. Copy and potentially modify options specific to this run
    ProgramOptions runOpts = m_lastRunOptions; // Make a copy for modification
    runOpts.generate_individual_plots = m_view->ShouldGenerateIndividualPlots(); // Get specific flag not stored in ArgumentManager

    // 5. Check for essential inputs (input files)
    if (runOpts.input_files.empty()) {
        m_view->ShowError(_("Error"), _("Please select at least one input RAW file."));
        return;
    }

    // 6. Filter calibration files from the input list for this run
    if (!runOpts.dark_file_path.empty() || !runOpts.sat_file_path.empty()) {
        std::set<std::string> calibration_files;
        if (!runOpts.dark_file_path.empty()) calibration_files.insert(runOpts.dark_file_path);
        if (!runOpts.sat_file_path.empty()) calibration_files.insert(runOpts.sat_file_path);

        // Remove calibration files directly from the runOpts copy
        runOpts.input_files.erase(
            std::remove_if(runOpts.input_files.begin(), runOpts.input_files.end(),
                [&](const std::string& input_file) {
                    return calibration_files.count(input_file);
                }),
            runOpts.input_files.end()
        );
        // Reflect removal in UI if necessary (optional, could cause flicker)
        // m_view->UpdateInputFileList(runOpts.input_files);
        // ArgumentManager might need update too if core relies on it directly later? Risky.
        // ArgumentManager::Instance().Set("input-files", runOpts.input_files);
        // UpdateCommandPreview(); // Update command preview if files changed
    }

    // 7. Set UI state to "processing"
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 1;
    m_view->SetUiState(true, num_threads);

    // 8. Ensure previous worker thread is finished before starting new one
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    // 9. Launch the worker thread with the prepared options copy
    m_cancelWorker = false; // Reset cancellation flag
    m_workerThread = std::thread([this, opts = std::move(runOpts)] { // Pass opts copy by value (move constructor)
        this->AnalysisWorker(opts);
        // Set running flag to false *after* thread finishes (or is about to exit)
        m_isWorkerRunning = false;
    });
}

void GuiPresenter::AddInputFiles(const std::vector<std::string>& files_to_add)
{
    if (files_to_add.empty()) return;
    wxBusyInfo wait(_("Loading and pre-processing files..."), m_view);
    wxTheApp->Yield();

    m_inputFileManager.AddFiles(files_to_add);

    std::vector<std::string> new_valid_files;
    for (const auto& file : files_to_add) {
        // Check against pre-analysis manager to avoid re-processing
        auto existing_files = m_preAnalysisManager.GetSortedFileList();
        if (std::find(existing_files.begin(), existing_files.end(), file) == existing_files.end()) {
            new_valid_files.push_back(file);
        }
    }

    if (new_valid_files.empty()) return;

    std::vector<std::future<PreAnalysisResult>> futures;
    double sat_value = m_view->GetSaturationValue();
    for (const auto& file : new_valid_files) {
        futures.push_back(std::async(std::launch::async, [file, sat_value]() -> PreAnalysisResult {
            RawFile raw_file(file);
            if (!raw_file.Load()) {
                return {file, -1.0, 0.0f, true, sat_value}; // Signal load failure
            }
           
             cv::Mat active_img = raw_file.GetActiveRawImage();
            if (active_img.empty()) {
                return {file, -1.0, 0.0f, true, sat_value}; // Signal invalid image
            }
            double mean_brightness = cv::mean(active_img)[0];
            int saturated_pixels = cv::countNonZero(active_img >= (sat_value * 0.99));
           
             return {file, mean_brightness, raw_file.GetIsoSpeed(), saturated_pixels > 0, sat_value};
        }));
    }

    std::vector<PreAnalysisResult> loaded_files;
    for (auto& fut : futures) {
        wxTheApp->Yield();
        auto result = fut.get();
        if (result.mean_brightness >= 0.0) { // Only keep successfully loaded files
            loaded_files.push_back(result);
        }
    }

    if (loaded_files.empty()) return;

    for (const auto& entry : loaded_files) {
        m_preAnalysisManager.AddFile(entry.filename, sat_value);
    }

    UpdateRawPreviewFromCache();
    UpdateCommandPreview();
}

void GuiPresenter::HandleGridCellClick()
{
    // Always display the summary image, if it's valid.
    if (m_summaryImage.IsOk()) {
        m_view->DisplayImage(m_summaryImage);
    }
}

void GuiPresenter::RemoveInputFiles(const std::vector<int>& indices)
{
    wxBusyInfo wait(_("Updating file list and preview..."), m_view);

    auto sorted_files = m_preAnalysisManager.GetSortedFileList();
    std::vector<std::string> files_to_remove;
    for (int index : indices) {
        if (index >= 0 && index < static_cast<int>(sorted_files.size())) {
            files_to_remove.push_back(sorted_files[index]);
        }
    }

    m_inputFileManager.RemoveFiles(files_to_remove);
    for (const auto& file : files_to_remove) {
        m_preAnalysisManager.RemoveFile(file);
    }

    UpdateRawPreviewFromCache();
    UpdateCommandPreview();
}

void GuiPresenter::RemoveAllInputFiles()
{
    wxBusyInfo wait(_("Updating file list and preview..."), m_view);
    m_inputFileManager.RemoveFiles(m_inputFileManager.GetInputFiles());
    m_preAnalysisManager.Clear();
    UpdateRawPreviewFromCache();
    UpdateCommandPreview();
}

const ProgramOptions& GuiPresenter::GetLastRunOptions() const { return m_lastRunOptions; }

const ReportOutput& GuiPresenter::GetLastReport() const { return m_lastReport; }

bool GuiPresenter::IsWorkerRunning() const { return m_isWorkerRunning; }

void GuiPresenter::RequestWorkerCancellation() { m_cancelWorker = true; }

const wxImage& GuiPresenter::GetLastSummaryImage() const { return m_summaryImage; }

void GuiPresenter::UpdateCalibrationFiles()
{
    m_inputFileManager.SetBlackFile(m_view->GetDarkFilePath());
    m_inputFileManager.SetSaturationFile(m_view->GetSaturationFilePath());
    
    // The list in PreAnalysisManager must be synced with the clean list
    auto clean_list = m_inputFileManager.GetInputFiles();
    auto current_list = m_preAnalysisManager.GetSortedFileList();
    std::sort(current_list.begin(), current_list.end());

    if (clean_list != current_list) {
        wxBusyInfo wait(_("Updating file list and preview..."), m_view);
        // Rebuild the pre-analysis manager from the clean list
        m_preAnalysisManager.Clear();
        double sat_value = m_view->GetSaturationValue();
        for (const auto& file : clean_list) {
            m_preAnalysisManager.AddFile(file, sat_value);
        }
        UpdateRawPreviewFromCache();
    }
    
    UpdateCommandPreview();
}

void GuiPresenter::UpdateRawPreviewFromCache()
{
    auto sorted_files = m_preAnalysisManager.GetSortedFileList();
    auto best_file_opt = m_preAnalysisManager.GetBestPreviewFile();

    std::string new_best_file = best_file_opt.has_value() ? best_file_opt.value() : "";

    // Only update the preview image if the file to display has changed.
    if (new_best_file != m_view->m_currentPreviewFile) { // Access public member of the frame
        m_view->UpdateRawPreview(new_best_file);
        m_view->m_currentPreviewFile = new_best_file; // Update the frame's state
    }

    // The file list is always updated to reflect deletions and the '▶' marker
    int display_index = -1;
    if (!new_best_file.empty()) {
        for (size_t i = 0; i < sorted_files.size(); ++i) {
            if (sorted_files[i] == new_best_file) {
                display_index = static_cast<int>(i);
                break;
            }
        }
    }
    
    m_view->UpdateInputFileList(sorted_files, display_index);
}

void GuiPresenter::OnExecuteButtonClicked()
{
    if (IsWorkerRunning()) {
        // If the worker is running, the button acts as a "Stop" button.
        RequestWorkerCancellation();
        
        // Change the UI to a "waiting to stop" state immediately.
        m_view->SetExecuteButtonToStoppingState();
    } else {
        // If the worker is not running, the button acts as an "Execute" button.
        StartAnalysis();
    }
}