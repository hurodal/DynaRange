// File: src/gui/controllers/ChartController.cpp
/**
 * @file gui/controllers/ChartController.cpp
 * @brief Implements the ChartController class.
 */
#include "ChartController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../helpers/ImageViewer.hpp"
#include "../../core/arguments/ChartOptionsParser.hpp" // For the struct
#include "../../core/graphics/ChartGenerator.hpp"
#include "../../core/utils/PathManager.hpp"
#include <wx/msgdlg.h>

ChartController::ChartController(DynaRangeFrame* frame) : m_frame(frame)
{
    m_thumbnailViewer = std::make_unique<ImageViewer>(m_frame->m_chartPreviewBitmap);
    
    // Bind the size event of the panel to our new handler.
    // This is the key to fixing the initial layout.
    m_frame->rightColChartPanel->Bind(wxEVT_SIZE, &ChartController::OnRightPanelSize, this);

    // Set initial values on UI controls from default options
    auto default_opts = ParseChartOptions(ProgramOptions(), std::cerr);
    if (default_opts) {
        m_frame->m_rParamSlider->SetValue(default_opts->R);
        m_frame->m_gParamSlider->SetValue(default_opts->G);
        m_frame->m_bParamSlider->SetValue(default_opts->B);
        m_frame->m_rParamValue->SetLabel(std::to_string(default_opts->R));
        m_frame->m_gParamValue->SetLabel(std::to_string(default_opts->G));
        m_frame->m_bParamValue->SetLabel(std::to_string(default_opts->B));
        m_frame->m_InvGammaValue->SetValue(wxString::Format("%.1f", default_opts->invgamma));
        m_frame->m_chartDimXValue->SetValue(std::to_string(default_opts->dim_x));
        m_frame->m_chartDimWValue->SetValue(std::to_string(default_opts->aspect_w));
        m_frame->m_chartDimHValue->SetValue(std::to_string(default_opts->aspect_h));
        m_frame->m_chartPatchRowValue->SetValue(std::to_string(default_opts->patches_m));
        m_frame->m_chartPatchColValue->SetValue(std::to_string(default_opts->patches_n));
    }
}

ChartController::~ChartController() = default;

int ChartController::GetChartPatchesM() const {
    long value = 0;
    m_frame->m_chartPatchRowValue->GetValue().ToLong(&value);
    return static_cast<int>(value);
}

int ChartController::GetChartPatchesN() const {
    long value = 0;
    m_frame->m_chartPatchColValue->GetValue().ToLong(&value);
    return static_cast<int>(value);
}

void ChartController::OnPreviewClick(wxCommandEvent& event) {
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();
    // Generate the thumbnail data in the core library
    std::optional<InMemoryImage> thumb_data_opt = GenerateChartThumbnail(opts, 256);
    if (thumb_data_opt) {
        // Convert the generic image data to a wxImage here, inside the GUI layer.
        InMemoryImage& thumb_data = *thumb_data_opt;
        wxImage image(thumb_data.width, thumb_data.height, thumb_data.data.data(), true); // true = static data
        m_thumbnailViewer->SetImage(image); // Use the viewer to handle setting and scaling
    }
}

void ChartController::OnCreateClick(wxCommandEvent& event) {
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();
    // Use a temporary ProgramOptions to leverage PathManager
    ProgramOptions temp_prog_opts; 
    PathManager paths(temp_prog_opts);
    fs::path chart_output_path = paths.GetCsvOutputPath().parent_path() / "magentachart_gui.png";

    if (GenerateTestChart(opts, chart_output_path.string(), std::cout)) {
        wxMessageBox(wxString::Format(_("Chart saved successfully to:\n%s"), chart_output_path.string()), _("Success"), wxOK | wxICON_INFORMATION);
    } else {
        wxMessageBox(_("Failed to generate and save the chart."), _("Error"), wxOK | wxICON_ERROR);
    }
}

void ChartController::OnColorSliderChanged(wxCommandEvent& event) {
    m_frame->m_rParamValue->SetLabel(std::to_string(m_frame->m_rParamSlider->GetValue()));
    m_frame->m_gParamValue->SetLabel(std::to_string(m_frame->m_gParamSlider->GetValue()));
    m_frame->m_bParamValue->SetLabel(std::to_string(m_frame->m_bParamSlider->GetValue()));
}

void ChartController::OnInputChanged(wxCommandEvent& event) {
    // This can be used later to trigger live preview if desired.
}

ChartGeneratorOptions ChartController::GetCurrentOptionsFromUi() const {
    ChartGeneratorOptions opts{};
    opts.R = m_frame->m_rParamSlider->GetValue();
    opts.G = m_frame->m_gParamSlider->GetValue();
    opts.B = m_frame->m_bParamSlider->GetValue();
    m_frame->m_InvGammaValue->GetValue().ToDouble(&opts.invgamma);
    long temp_val;
    m_frame->m_chartDimXValue->GetValue().ToLong(&temp_val); opts.dim_x = temp_val;
    m_frame->m_chartDimWValue->GetValue().ToLong(&temp_val); opts.aspect_w = temp_val;
    m_frame->m_chartDimHValue->GetValue().ToLong(&temp_val); opts.aspect_h = temp_val;
    opts.patches_m = GetChartPatchesM();
    opts.patches_n = GetChartPatchesN();
    return opts;
}

void ChartController::OnRightPanelSize(wxSizeEvent& event) {
    // Schedule the update for the next event cycle. This ensures the panel's
    // layout has finished calculating and the image is repainted correctly.
    m_frame->CallAfter([this]() {
        if (m_thumbnailViewer) {
            m_thumbnailViewer->HandleResize();
        }
    });
    // Propagate the event to allow other layout calculations to proceed.
    event.Skip();
}