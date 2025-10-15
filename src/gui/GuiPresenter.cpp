// File: gui/GuiPresenter.cpp
/**
 * @file gui/GuiPresenter.cpp
 * @brief Implements the application logic presenter for the GUI.
 */
#include "GuiPresenter.hpp"
#include "DynaRangeFrame.hpp" // Include the full View definition
#include "helpers/GuiPlotter.hpp"
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/utils/CommandGenerator.hpp"
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

namespace fs = std::filesystem;

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
    m_currentPreviewFile = "";
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
    
    // Sincroniza la lista de ficheros desde el gestor de pre-análisis al gestor de argumentos.
    mgr.Set(InputFiles, m_preAnalysisManager.GetSortedFileList());

    mgr.Set(BlackFile, m_view->GetDarkFilePath());
    mgr.Set(SaturationFile, m_view->GetSaturationFilePath());
    mgr.Set(BlackLevel, m_view->GetDarkValue());
    mgr.Set(SaturationLevel, m_view->GetSaturationValue());
    mgr.Set(PatchRatio, m_view->GetPatchRatio());
    mgr.Set(OutputFile, m_view->GetOutputFilePath());
    mgr.Set(SnrThresholdDb, m_view->GetSnrThresholds());
    mgr.Set(DrNormalizationMpx, m_view->GetDrNormalization());
    mgr.Set(PolyFit, m_view->GetPolyOrder());

    int plotModeChoice = m_view->GetPlotMode();
    bool generatePlot = (plotModeChoice != 0);
    mgr.Set(GeneratePlot, generatePlot);

    if (generatePlot) {
        DynaRange::Graphics::Constants::PlotOutputFormat format_enum = m_view->GetPlotFormat();
        std::string format_str = "PNG";
        if (format_enum == DynaRange::Graphics::Constants::PlotOutputFormat::PDF) {
            format_str = "PDF";
        } else if (format_enum == DynaRange::Graphics::Constants::PlotOutputFormat::SVG) {
            format_str = "SVG";
        }
        mgr.Set(PlotFormat, format_str);

        PlottingDetails details = m_view->GetPlottingDetails();
        int commandMode = plotModeChoice;
        // 1: No cmd, 2: Short, 3: Long
        std::vector<int> params = { static_cast<int>(details.show_scatters), static_cast<int>(details.show_curve), static_cast<int>(details.show_labels), commandMode };
        mgr.Set(PlotParams, params);
    }

    mgr.Set(ChartCoords, m_view->GetChartCoords());
    
    std::vector<int> patches = { m_view->GetChartPatchesM(), m_view->GetChartPatchesN() };
    mgr.Set(ChartPatches, patches);

    RawChannelSelection channels = m_view->GetRawChannelSelection();
    std::vector<int> channels_vec = { 
        static_cast<int>(channels.R), 
        static_cast<int>(channels.G1), 
        static_cast<int>(channels.G2), 
        static_cast<int>(channels.B), 
        static_cast<int>(channels.avg_mode) 
    };
    mgr.Set(RawChannels, channels_vec);

    bool black_is_default = m_view->GetDarkFilePath().empty();
    mgr.Set(BlackLevelIsDefault, black_is_default);
    bool sat_is_default = m_view->GetSaturationFilePath().empty();
    mgr.Set(SaturationLevelIsDefault, sat_is_default);
    
    std::vector<double> current_thresholds = m_view->GetSnrThresholds();
    bool snr_is_default = (current_thresholds.size() == 2 && current_thresholds[0] == 0 && current_thresholds[1] == 12);
    mgr.Set(SnrThresholdIsDefault, snr_is_default);
    
    mgr.Set(PrintPatches, m_view->GetPrintPatchesFilename());
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
    if (!m_view->ValidateSnrThresholds()) {
        m_view->ShowError(_("Invalid Input"), _("The 'SNR Thresholds' field contains invalid characters. Please enter only numbers separated by spaces."));
        return;
    }

    UpdateManagerFromView();

    m_lastRunOptions = ArgumentManager::Instance().ToProgramOptions();
    
    m_lastRunOptions.generate_individual_plots = m_view->ShouldGenerateIndividualPlots();

    if (m_lastRunOptions.input_files.empty()) {
        m_view->ShowError(_("Error"), _("Please select at least one input RAW file."));
        return;
    }
    
    // The source image selection logic is now handled inside RunDynamicRangeAnalysis,
    // so it is no longer needed here. We just pass the options.

    if (!m_lastRunOptions.dark_file_path.empty() || !m_lastRunOptions.sat_file_path.empty()) {
        std::set<std::string> calibration_files;
        if (!m_lastRunOptions.dark_file_path.empty()) {
            calibration_files.insert(m_lastRunOptions.dark_file_path);
        }
        if (!m_lastRunOptions.sat_file_path.empty()) {
            calibration_files.insert(m_lastRunOptions.sat_file_path);
        }

        m_lastRunOptions.input_files.erase(
            std::remove_if(m_lastRunOptions.input_files.begin(), m_lastRunOptions.input_files.end(),
                [&](const std::string& input_file) {
                    return calibration_files.count(input_file);
                }),
            m_lastRunOptions.input_files.end()
        );
        m_view->UpdateInputFileList(m_lastRunOptions.input_files);
        ArgumentManager::Instance().Set("input-files", m_lastRunOptions.input_files);
        UpdateCommandPreview();
    }

    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 1; 
    }
    m_view->SetUiState(true, num_threads);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_cancelWorker = false;
    m_workerThread = std::thread([this] {
        this->AnalysisWorker(m_lastRunOptions);
        m_isWorkerRunning = false;
    });
}

void GuiPresenter::AnalysisWorker(ProgramOptions opts)
{
    m_isWorkerRunning = true;
    // Clear previous results
    m_summaryImage = wxImage();
    m_individualImages.clear();
    
    WxLogStreambuf log_streambuf(m_view);
    std::ostream log_stream(&log_streambuf);

    // 1. Run core analysis to get raw data and file-based reports
    m_lastReport = DynaRange::RunDynamicRangeAnalysis(opts, log_stream, m_cancelWorker);
    
    if (m_cancelWorker) {
        if (m_view)
            m_view->PostAnalysisComplete();
        return;
    }

    // 2. After core analysis, generate in-memory images for the GUI
    if (opts.generate_plot && !m_lastReport.curve_data.empty()) {
        log_stream << _("\nGenerating in-memory plots for GUI...") << std::endl;

        // Create the ReportingParameters struct needed by the plotter.
        ReportingParameters reporting_params {
            .raw_channels = opts.raw_channels,
            .generate_plot = opts.generate_plot,
            .plot_format = opts.plot_format,
            .plot_details = opts.plot_details,
            .plot_command_mode = opts.plot_command_mode,
            .generated_command = m_lastReport.curve_data[0].generated_command, // Get from results
            .dark_value = opts.dark_value,
            .saturation_value = opts.saturation_value,
            .black_level_is_default = opts.black_level_is_default,
            .saturation_level_is_default = opts.saturation_level_is_default,
            .snr_thresholds_db = opts.snr_thresholds_db
        };
        
        // Generate summary plot image
        wxString summary_title_wx = _("SNR Curves - Summary (") + wxString(m_lastReport.curve_data[0].camera_model) + ")";
        std::string summary_title = std::string(summary_title_wx.mb_str());
        m_summaryImage = GuiPlotter::GeneratePlotAsWxImage(m_lastReport.curve_data, m_lastReport.dr_results, summary_title, reporting_params);
        
        // Generate individual plot images
        std::map<std::string, std::vector<CurveData>> curves_by_file;
        for (const auto& curve : m_lastReport.curve_data) {
            curves_by_file[curve.filename].push_back(curve);
        }
        std::map<std::string, std::vector<DynamicRangeResult>> results_by_file;
        for (const auto& result : m_lastReport.dr_results) {
            results_by_file[result.filename].push_back(result);
        }

        for (const auto& pair : curves_by_file) {
            const std::string& filename = pair.first;
            std::stringstream title_ss;
            title_ss << fs::path(filename).filename().string() << " (" << pair.second[0].camera_model << ")";
            m_individualImages[filename] = GuiPlotter::GeneratePlotAsWxImage(pair.second, results_by_file[filename], title_ss.str(), reporting_params);
        }
        log_stream << _("In-memory plot generation complete.") << std::endl;
    }

    // 3. Notify the view on the main thread that all work is done
    if (m_view) {
        m_view->PostAnalysisComplete();
    }
}

void GuiPresenter::AddInputFiles(const std::vector<std::string>& files_to_add)
{
    if (files_to_add.empty()) return;

    // Show a busy indicator window that will be automatically destroyed on scope exit (RAII).
    wxBusyInfo wait(_("Loading and pre-processing files..."), m_view);
    // Force the UI to update and show the busy info immediately.
    wxTheApp->Yield();

    std::set<std::string> calibration_files;
    std::string dark_file = m_view->GetDarkFilePath();
    if (!dark_file.empty()) {
        calibration_files.insert(dark_file);
    }
    std::string sat_file = m_view->GetSaturationFilePath();
    if (!sat_file.empty()) {
        calibration_files.insert(sat_file);
    }

    std::vector<std::string> new_valid_files;
    std::set<std::string> existing_files(m_inputFiles.begin(), m_inputFiles.end());
    for (const auto& file : files_to_add) {
        bool is_calibration_file = calibration_files.count(file) > 0;
        bool is_already_in_list = existing_files.count(file) > 0;
        if (!is_calibration_file && !is_already_in_list) {
            new_valid_files.push_back(file);
        }
    }

    if (new_valid_files.empty()) return;

    // --- ASYNCHRONOUS LOADING PHASE ---
    // Launch a future for each file to load it in a background thread.
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

    // Wait for all futures to complete, periodically yielding to keep the UI responsive.
    std::vector<PreAnalysisResult> loaded_files;
    for (auto& fut : futures) {
        wxTheApp->Yield();
        auto result = fut.get();
        if (result.mean_brightness >= 0.0) { // Only keep successfully loaded files
            loaded_files.push_back(result);
        }
    }

    // If no files were successfully loaded, abort.
    if (loaded_files.empty()) return;

    // Add the new entries to the manager.
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

    for (const auto& file : files_to_remove) {
        m_preAnalysisManager.RemoveFile(file);
    }

    UpdateRawPreviewFromCache();
    UpdateCommandPreview();
}

void GuiPresenter::RemoveAllInputFiles()
{
    wxBusyInfo wait(_("Updating file list and preview..."), m_view);
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
    std::string dark_file = m_view->GetDarkFilePath();
    std::string sat_file = m_view->GetSaturationFilePath();
    std::set<std::string> calibration_files;
    if (!dark_file.empty()) {
        calibration_files.insert(dark_file);
    }
    if (!sat_file.empty()) {
        calibration_files.insert(sat_file);
    }
    if (!calibration_files.empty()) {
        auto original_size = m_preAnalysisManager.GetSortedFileList().size();
        for (const auto& file : calibration_files) {
            m_preAnalysisManager.RemoveFile(file);
        }
        if (m_preAnalysisManager.GetSortedFileList().size() != original_size) {
            wxBusyInfo wait(_("Updating file list and preview..."), m_view);
            UpdateRawPreviewFromCache();
        }
    }
    UpdateCommandPreview();
}

void GuiPresenter::UpdateRawPreview()
{
    if (m_inputFiles.empty()) {
        m_view->UpdateRawPreview(""); // Clear preview
        m_view->UpdateInputFileList(m_inputFiles, -1);
        return;
    }
    // Use the new core pre-analysis function.
    // We need a saturation value. We'll get it from the view for now.
    // In the future, this could be part of a more robust state management.
    double sat_value = m_view->GetSaturationValue();
    auto pre_analysis_results = PreAnalyzeRawFiles(m_inputFiles, sat_value, nullptr);
    if (pre_analysis_results.empty()) {
        m_view->UpdateRawPreview(""); // Clear preview if no files could be read
        m_view->UpdateInputFileList(m_inputFiles, -1);
        return;
    }
    // Find the best file index in the pre_analysis_results vector.
    int best_index = 0;
    bool found_non_saturated = false;
    // Iterate from the end (brightest) to find the first non-saturated file.
    for (int i = static_cast<int>(pre_analysis_results.size()) - 1; i >= 0; --i) {
        if (!pre_analysis_results[i].has_saturated_pixels) {
            best_index = i;
            found_non_saturated = true;
            break;
        }
    }
    if (!found_non_saturated) {
        best_index = 0; // Use the darkest file.
    }
    std::string best_file_path = pre_analysis_results[best_index].filename;
    m_view->UpdateRawPreview(best_file_path);
    // Find the index in the original m_inputFiles list.
    int display_index = -1;
    for (size_t i = 0; i < m_inputFiles.size(); ++i) {
        if (m_inputFiles[i] == best_file_path) {
            display_index = static_cast<int>(i);
            break;
        }
    }
    m_view->UpdateInputFileList(m_inputFiles, display_index);
}

void GuiPresenter::UpdateRawPreviewFromCache()
{
    auto sorted_files = m_preAnalysisManager.GetSortedFileList();
    auto best_file_opt = m_preAnalysisManager.GetBestPreviewFile();

    // Determina la ruta del nuevo fichero a mostrar (o una cadena vacía si no hay ninguno)
    std::string new_best_file = best_file_opt.has_value() ? best_file_opt.value() : "";

    // --- LÓGICA INTELIGENTE AÑADIDA ---
    // Solo actualiza la imagen de previsualización si el fichero a mostrar ha cambiado.
    if (new_best_file != m_currentPreviewFile) {
        m_view->UpdateRawPreview(new_best_file);
        m_currentPreviewFile = new_best_file;
    }

    // La lista de ficheros siempre se actualiza para reflejar borrados y la marca '▶'
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