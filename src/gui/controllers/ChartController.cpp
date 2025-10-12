// File: src/gui/controllers/ChartController.cpp
/**
 * @file gui/controllers/ChartController.cpp
 * @brief Implements the ChartController class.
 */
#include "ChartController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../../core/arguments/ChartOptionsParser.hpp"
#include "../../core/graphics/ChartGenerator.hpp"
#include "../../core/utils/PathManager.hpp"
#include "../../gui/Constants.hpp"
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/bitmap.h>
#include <wx/image.h>

ChartController::ChartController(DynaRangeFrame* frame) : m_frame(frame)
{
    // --- Set initial values in the UI controls ---
    ChartGeneratorOptions default_opts{};
    m_frame->m_rParamSlider->SetValue(default_opts.R);
    m_frame->m_gParamSlider->SetValue(default_opts.G);
    m_frame->m_bParamSlider->SetValue(default_opts.B);
    m_frame->m_rParamValue->SetLabel(std::to_string(default_opts.R));
    m_frame->m_gParamValue->SetLabel(std::to_string(default_opts.G));
    m_frame->m_bParamValue->SetLabel(std::to_string(default_opts.B));
    m_frame->m_InvGammaValue->SetValue(wxString::Format("%.1f", default_opts.invgamma));
    m_frame->m_chartDimXValue->SetValue(std::to_string(default_opts.dim_x));
    m_frame->m_chartDimWValue->SetValue(std::to_string(default_opts.aspect_w));
    m_frame->m_chartDimHValue->SetValue(std::to_string(default_opts.aspect_h));
    m_frame->m_chartPatchRowValue->SetValue(std::to_string(default_opts.patches_m));
    m_frame->m_chartPatchColValue->SetValue(std::to_string(default_opts.patches_n));
    m_frame->m_chartPatchRowValue1->SetValue(std::to_string(default_opts.patches_m));
    m_frame->m_chartPatchColValue1->SetValue(std::to_string(default_opts.patches_n));
}

ChartController::~ChartController() = default;

void ChartController::OnPreviewClick(wxCommandEvent& event) {
    // 1. Get current options from the UI.
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();

    // 2. Generate the thumbnail image data in memory.
    std::optional<InMemoryImage> thumb_data_opt = GenerateChartThumbnail(opts, DynaRange::Gui::Constants::CHART_PREVIEW_WIDTH);

    if (thumb_data_opt) {
        const InMemoryImage& thumb_data = *thumb_data_opt;
        
        // 3. Convert the raw RGB data to a wxImage.
        wxImage image(thumb_data.width, thumb_data.height, const_cast<unsigned char*>(thumb_data.data.data()), true);
        
        if (image.IsOk()) {
            // 4. Store the wxBitmap in the frame and trigger a redraw.
            m_frame->m_chartPreviewBitmap = wxBitmap(image);
            m_frame->m_chartPreviewPanel->Refresh();
        } else {
            wxLogError("Failed to create wxImage for chart preview.");
            m_frame->m_chartPreviewBitmap = wxBitmap(); // Clear bitmap
            m_frame->m_chartPreviewPanel->Refresh();
        }
    } else {
        wxLogError("Failed to generate chart preview thumbnail.");
        m_frame->m_chartPreviewBitmap = wxBitmap(); // Clear bitmap
        m_frame->m_chartPreviewPanel->Refresh();
    }
}

void ChartController::OnCreateClick(wxCommandEvent& event) {
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();
    ProgramOptions temp_prog_opts; 
    PathManager paths(temp_prog_opts);
    fs::path chart_output_path = paths.GetCsvOutputPath().parent_path() / "magentachart_gui.png";
    if (GenerateTestChart(opts, chart_output_path.string(), std::cout)) {
        wxMessageBox(wxString::Format(_("Chart saved successfully to:%s"), chart_output_path.string()), _("Success"), wxOK | wxICON_INFORMATION);
    } else {
        wxMessageBox(_("Failed to generate and save the chart."), _("Error"), wxOK | wxICON_ERROR);
    }
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
    opts.patches_m = m_frame->GetChartPatchesM();
    opts.patches_n = m_frame->GetChartPatchesN();
    return opts;
}

void ChartController::OnColorSliderChanged(wxCommandEvent& event) {
    m_frame->m_rParamValue->SetLabel(std::to_string(m_frame->m_rParamSlider->GetValue()));
    m_frame->m_gParamValue->SetLabel(std::to_string(m_frame->m_gParamSlider->GetValue()));
    m_frame->m_bParamValue->SetLabel(std::to_string(m_frame->m_bParamSlider->GetValue()));
}

void ChartController::OnInputChanged(wxCommandEvent& event) {
    // This can be used later to trigger live preview if desired.
}

void ChartController::OnChartChartPatchChanged(wxCommandEvent& event) {
    if (m_frame->m_isUpdatingPatches) return;
    m_frame->m_isUpdatingPatches = true;

    m_frame->m_chartPatchRowValue1->ChangeValue(m_frame->m_chartPatchRowValue->GetValue());
    m_frame->m_chartPatchColValue1->ChangeValue(m_frame->m_chartPatchColValue->GetValue());
    
    // This event doesn't affect the CLI command, so no need to call UpdateCommandPreview.
    
    m_frame->m_isUpdatingPatches = false;
    event.Skip();
}