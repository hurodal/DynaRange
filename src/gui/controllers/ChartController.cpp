// File: src/gui/controllers/ChartController.cpp
/**
 * @file gui/controllers/ChartController.cpp
 * @brief Implements the ChartController class.
 */
#include "ChartController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../../core/arguments/ChartOptionsParser.hpp" // For the struct
#include "../../core/graphics/ChartGenerator.hpp"
#include "../../core/utils/PathManager.hpp"
#include <wx/msgdlg.h>
#include <algorithm>

ChartController::ChartController(DynaRangeFrame* frame) : m_frame(frame)
{
    m_frame->rightColChartPanel->Bind(wxEVT_SIZE, &ChartController::OnRightPanelSize, this);
    m_frame->m_chartPreviewBitmap->SetBitmap(wxNullBitmap); // Clear bitmap initially

    // Set initial values on UI controls directly from a default-initialized
    // ChartGeneratorOptions struct, which now holds the central default values.
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
    // Initialize both sets of patch controls from the central source of truth.
    m_frame->m_chartPatchRowValue->SetValue(std::to_string(default_opts.patches_m));
    m_frame->m_chartPatchColValue->SetValue(std::to_string(default_opts.patches_n));
    m_frame->m_chartPatchRowValue1->SetValue(std::to_string(default_opts.patches_m));
    m_frame->m_chartPatchColValue1->SetValue(std::to_string(default_opts.patches_n));
}

ChartController::~ChartController()
{
    // Unbind the event to prevent issues on shutdown
    if (m_frame && m_frame->rightColChartPanel) {
        // Corrected typo from UnBind to Unbind
        m_frame->rightColChartPanel->Unbind(wxEVT_SIZE, &ChartController::OnRightPanelSize, this);
    }
}

void ChartController::OnPreviewClick(wxCommandEvent& event) {
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();
    // Generate the thumbnail data in the core library
    std::optional<InMemoryImage> thumb_data_opt = GenerateChartThumbnail(opts, 256);
    if (thumb_data_opt) {
        // Convert the generic image data to a wxImage here, inside the GUI layer.
        InMemoryImage& thumb_data = *thumb_data_opt;
        m_chart_preview_image = wxImage(thumb_data.width, thumb_data.height, thumb_data.data.data(), true); // true = static data
        UpdateBitmapDisplay(); // Use the new helper to scale and display
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

    // Calls are now correctly routed through the m_frame pointer.
    opts.patches_m = m_frame->GetChartPatchesM();
    opts.patches_n = m_frame->GetChartPatchesN();
    return opts;
}

void ChartController::OnRightPanelSize(wxSizeEvent& event) {
    m_frame->CallAfter([this]() {
        UpdateBitmapDisplay();
    });
    event.Skip();
}

void DynaRangeFrame::OnInputChartPatchChanged(wxCommandEvent& event) {
    if (m_isUpdatingPatches) return;
    m_isUpdatingPatches = true;

    m_chartPatchRowValue->ChangeValue(m_chartPatchRowValue1->GetValue());
    m_chartPatchColValue->ChangeValue(m_chartPatchColValue1->GetValue());
    
    m_presenter->UpdateCommandPreview();
    m_isUpdatingPatches = false;
    event.Skip();
}

void DynaRangeFrame::OnChartChartPatchChanged(wxCommandEvent& event) {
    if (m_isUpdatingPatches) return;
    m_isUpdatingPatches = true;

    m_chartPatchRowValue1->ChangeValue(m_chartPatchRowValue->GetValue());
    m_chartPatchColValue1->ChangeValue(m_chartPatchColValue->GetValue());

    m_presenter->UpdateCommandPreview();
    m_isUpdatingPatches = false;
    event.Skip();
}

void ChartController::UpdateBitmapDisplay() {
    if (!m_chart_preview_image.IsOk() || !m_frame || !m_frame->m_chartPreviewBitmap || !m_frame->m_chartPreviewBitmap->GetParent()) { 
        return;
    }

    wxSize containerSize = m_frame->m_chartPreviewBitmap->GetParent()->GetClientSize();
    if (containerSize.GetWidth() <= 0 || containerSize.GetHeight() <= 0) {
        return;
    }

    wxImage imageCopy = m_chart_preview_image.Copy();
    int imgWidth = imageCopy.GetWidth();
    int imgHeight = imageCopy.GetHeight();

    double hScale = static_cast<double>(containerSize.GetWidth()) / imgWidth;
    double vScale = static_cast<double>(containerSize.GetHeight()) / imgHeight;
    double scale = std::min(hScale, vScale);

    imageCopy.Rescale(imgWidth * scale, imgHeight * scale, wxIMAGE_QUALITY_HIGH);

    m_frame->m_chartPreviewBitmap->SetBitmap(wxBitmap(imageCopy));
    if (m_frame->m_chartPreviewBitmap->GetParent()) {
        m_frame->m_chartPreviewBitmap->GetParent()->Layout();
    }
}