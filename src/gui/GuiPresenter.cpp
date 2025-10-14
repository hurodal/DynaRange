// File: gui/GuiPresenter.cpp
/**
 * @file gui/GuiPresenter.cpp
 * @brief Implements the application logic presenter for the GUI.
 */
#include "GuiPresenter.hpp"
#include "../core/arguments/ArgumentManager.hpp"
#include "../core/engine/Engine.hpp"
#include "../core/utils/CommandGenerator.hpp"
#include "../core/arguments/Constants.hpp"
#include "DynaRangeFrame.hpp" // Include the full View definition
#include "helpers/GuiPlotter.hpp" // Include the new GUI plotter
#include <algorithm>
#include <filesystem>
#include <ostream>
#include <set>
#include <streambuf>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
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
    
    mgr.Set(BlackFile, m_view->GetDarkFilePath());
    mgr.Set(SaturationFile, m_view->GetSaturationFilePath());
    mgr.Set(BlackLevel, m_view->GetDarkValue());
    mgr.Set(SaturationLevel, m_view->GetSaturationValue());
    mgr.Set(PatchRatio, m_view->GetPatchRatio());
    mgr.Set(InputFiles, m_view->GetInputFiles());
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
        int commandMode = plotModeChoice; // 1: No cmd, 2: Short, 3: Long
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
    // Get the current calibration files to build an exclusion list.
    std::set<std::string> calibration_files;
    std::string dark_file = m_view->GetDarkFilePath();
    if (!dark_file.empty()) {
        calibration_files.insert(dark_file);
    }
    std::string sat_file = m_view->GetSaturationFilePath();
    if (!sat_file.empty()) {
        calibration_files.insert(sat_file);
    }

    // Create a temporary list for files that are valid to add.
    std::vector<std::string> new_valid_files;
    // Use a set of existing files for efficient lookup.
    std::set<std::string> existing_files(m_inputFiles.begin(), m_inputFiles.end());

    for (const auto& file : files_to_add) {
        bool is_calibration_file = calibration_files.count(file) > 0;
        bool is_already_in_list = existing_files.count(file) > 0;

        // A file is valid to be added only if it's not a calibration file
        // AND it's not already in the input list.
        if (!is_calibration_file && !is_already_in_list) {
            new_valid_files.push_back(file);
        }
    }

    // If there are any new valid files, update the state and the view.
    if (!new_valid_files.empty()) {
        // Update the internal state first by appending the new files.
        m_inputFiles.insert(m_inputFiles.end(), new_valid_files.begin(), new_valid_files.end());
        std::sort(m_inputFiles.begin(), m_inputFiles.end());

        // Update the view from the single source of truth.
        m_view->UpdateInputFileList(m_inputFiles);
        UpdateCommandPreview();
    }
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
    // It is CRITICAL to delete items from the end to the beginning
    // to avoid invalidating the indices of the remaining items.
    std::vector<int> sorted_indices = indices;
    std::sort(sorted_indices.rbegin(), sorted_indices.rend());

    for (int index : sorted_indices) {
        if (index < m_inputFiles.size()) {
            m_inputFiles.erase(m_inputFiles.begin() + index);
        }
    }

    // Update the view from the single source of truth.
    m_view->UpdateInputFileList(m_inputFiles);
    UpdateCommandPreview();
}

void GuiPresenter::RemoveAllInputFiles()
{
    m_inputFiles.clear();
    // Update the view from the single source of truth.
    m_view->UpdateInputFileList(m_inputFiles);
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
        // Perform the filtering on the internal state (the single source of truth).
        auto original_size = m_inputFiles.size();
        m_inputFiles.erase(
            std::remove_if(m_inputFiles.begin(), m_inputFiles.end(),
                [&](const std::string& input_file) {
                    return calibration_files.count(input_file) > 0;
                }),
            m_inputFiles.end()
        );

        // Only update the view if a change actually occurred.
        if (m_inputFiles.size() != original_size) {
            m_view->UpdateInputFileList(m_inputFiles);
        }
    }

    // Always update the command preview to reflect the current state.
    UpdateCommandPreview();
}