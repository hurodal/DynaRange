// File: src/gui/controllers/ChartController.cpp
/**
 * @file gui/controllers/ChartController.cpp
 * @brief Implements the ChartController class.
 */
#include "ChartController.hpp"
#include "InputController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../GuiPresenter.hpp" 
#include "../../core/arguments/ChartOptionsParser.hpp"
#include "../../core/arguments/ArgumentsOptions.hpp"
#include "../../core/artifacts/ArtifactFactory.hpp" 
#include "../../core/utils/PathManager.hpp"
#include "../../core/utils/OutputNamingContext.hpp"
#include "../../core/utils/OutputFilenameGenerator.hpp"
#include "../../gui/Constants.hpp"
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <sstream>     
#include <filesystem>   

namespace fs = std::filesystem; // Alias for filesystem

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
    // Sincronizar también los controles de la pestaña "Input Chart Manual Corners"
    m_frame->m_chartPatchRowValue1->SetValue(std::to_string(default_opts.patches_m));
    m_frame->m_chartPatchColValue1->SetValue(std::to_string(default_opts.patches_n));

}

ChartController::~ChartController() = default;

void ChartController::OnChartChartPatchChanged(wxCommandEvent& event) {
    // Evitar recursión si estamos actualizando programáticamente
    if (m_frame->m_isUpdatingPatches) return;
    m_frame->m_isUpdatingPatches = true;

    // Sincronizar desde Chart tab -> Input tab
    m_frame->m_chartPatchRowValue1->ChangeValue(m_frame->m_chartPatchRowValue->GetValue());
    m_frame->m_chartPatchColValue1->ChangeValue(m_frame->m_chartPatchColValue->GetValue());
    // Actualizar preview y comando
    UpdatePreview();
    // Acceder al presenter a través del frame para actualizar el comando
    if(m_frame->m_presenter) { // Comprobación por seguridad
        m_frame->m_presenter->UpdateCommandPreview();
    }

    m_frame->m_isUpdatingPatches = false;
    event.Skip();
}

void ChartController::OnCreateClick(wxCommandEvent& event) {
    // 1. Get chart generation parameters from the UI
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();

    // 2. Determine effective camera name using InputController's logic
    std::string effective_camera_name = "";
    if (m_frame && m_frame->GetInputController()) { // Check pointers
        effective_camera_name = m_frame->GetInputController()->DetermineEffectiveCameraName();
    }

    // 3. Create context for filename generation, passing the effective name
    OutputNamingContext naming_ctx;
    naming_ctx.effective_camera_name_for_output = effective_camera_name;
    // Populate other necessary fields if any (currently none for this specific filename)

    // 4. Determine the save path using PathManager based on current settings
    ProgramOptions temp_prog_opts;
    // Use the current output CSV path setting from InputController to ensure consistency
    if (m_frame && m_frame->GetInputController()) {
        temp_prog_opts.output_filename = m_frame->GetInputController()->GetOutputFilePath();
        if (temp_prog_opts.output_filename.empty()){
            temp_prog_opts.output_filename = DEFAULT_OUTPUT_FILENAME;
        }
    } else {
         temp_prog_opts.output_filename = DEFAULT_OUTPUT_FILENAME; // Fallback
    }
    PathManager paths(temp_prog_opts);

    // 5. Use ArtifactFactory to create and save the chart
    std::stringstream log_stream_buffer; // Use a stringstream to capture logs
    std::optional<fs::path> chart_path_opt = ArtifactFactory::CreateTestChartImage(
        opts,
        naming_ctx,
        paths,
        log_stream_buffer // Pass the stringstream for logging
    );

    // 6. Notify the user
    if (chart_path_opt) {
        wxMessageBox(wxString::Format(_("Chart saved successfully to:\n%s"), chart_path_opt->wstring()),
                     _("Chart Saved"), wxOK | wxICON_INFORMATION, m_frame);
    } else {
        // Log the captured error message from the stringstream
        wxLogError("Chart generation failed. Log: %s", log_stream_buffer.str());
        // Show a message box indicating failure (path might be invalid if factory failed early)
        // We generate a potential path just for the error message using the filename generator directly
        fs::path potential_filename = OutputFilenameGenerator::GenerateTestChartFilename(naming_ctx);
        fs::path potential_output_path = paths.GetFullPath(potential_filename);
        wxMessageBox(wxString::Format(_("Failed to save the chart to:\n%s\n\nCheck application log for details."), potential_output_path.wstring()),
                     _("Error Saving Chart"), wxOK | wxICON_ERROR, m_frame);
    }
}
ChartGeneratorOptions ChartController::GetCurrentOptionsFromUi() const {
    ChartGeneratorOptions opts{}; // Inicializa con valores por defecto del struct

    // Leer valores de los controles
    opts.R = m_frame->m_rParamSlider->GetValue();
    opts.G = m_frame->m_gParamSlider->GetValue();
    opts.B = m_frame->m_bParamSlider->GetValue();

    // Usar conversión segura con comprobación y fallback a las constantes
    double invgamma_read;
    if (m_frame->m_InvGammaValue->GetValue().ToDouble(&invgamma_read) && invgamma_read > 0.0) {
        opts.invgamma = invgamma_read;
    } else {
        opts.invgamma = DEFAULT_CHART_INV_GAMMA;
    }

    long temp_val;
    if (m_frame->m_chartDimXValue->GetValue().ToLong(&temp_val) && temp_val > 0) {
        opts.dim_x = temp_val;
    } else {
        opts.dim_x = DEFAULT_CHART_DIM_X;
    }

    if (m_frame->m_chartDimWValue->GetValue().ToLong(&temp_val) && temp_val > 0) {
        opts.aspect_w = temp_val;
    } else {
        opts.aspect_w = DEFAULT_CHART_ASPECT_W;
    }

    if (m_frame->m_chartDimHValue->GetValue().ToLong(&temp_val) && temp_val > 0) {
        opts.aspect_h = temp_val;
    } else {
        opts.aspect_h = DEFAULT_CHART_ASPECT_H;
    }

    // Leer desde los controles de esta pestaña (Chart)
    if (m_frame->m_chartPatchRowValue->GetValue().ToLong(&temp_val) && temp_val > 0) {
        opts.patches_m = temp_val;
    } else {
        opts.patches_m = DEFAULT_CHART_PATCHES_M;
    }

    if (m_frame->m_chartPatchColValue->GetValue().ToLong(&temp_val) && temp_val > 0) {
        opts.patches_n = temp_val;
    } else {
        opts.patches_n = DEFAULT_CHART_PATCHES_N;
    }

    return opts;
}

void ChartController::OnColorSliderChanged(wxCommandEvent& event) {
    m_frame->m_rParamValue->SetLabel(std::to_string(m_frame->m_rParamSlider->GetValue()));
    m_frame->m_gParamValue->SetLabel(std::to_string(m_frame->m_gParamSlider->GetValue()));
    m_frame->m_bParamValue->SetLabel(std::to_string(m_frame->m_bParamSlider->GetValue()));
    UpdatePreview(); // Actualizar preview en tiempo real
    event.Skip();
}

void ChartController::OnChartParamTextChanged(wxCommandEvent& event) {
    // Podríamos añadir validación aquí si quisiéramos dar feedback inmediato
    // sobre entradas inválidas, pero por ahora solo actualizamos la preview.
    UpdatePreview();
    event.Skip();
}

void ChartController::OnChartPreviewPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(m_frame->m_chartPreviewPanel);
    dc.Clear();
    if (m_frame->m_chartPreviewBitmap.IsOk())
    {
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if(gc)
        {
            double bmp_w = m_frame->m_chartPreviewBitmap.GetWidth();
            double bmp_h = m_frame->m_chartPreviewBitmap.GetHeight();
            const wxSize panel_size = dc.GetSize();

            // Calcular escalado con aspecto preservado
            double scale_factor = 1.0;
            if (bmp_w > 0 && bmp_h > 0) {
                 scale_factor = std::min((double)panel_size.GetWidth() / bmp_w, (double)panel_size.GetHeight() / bmp_h);
            }
            double final_width = bmp_w * scale_factor;
            double final_height = bmp_h * scale_factor;
            double offset_x = (panel_size.GetWidth() - final_width) / 2.0;
            double offset_y = (panel_size.GetHeight() - final_height) / 2.0;

            gc->DrawBitmap(m_frame->m_chartPreviewBitmap, offset_x, offset_y, final_width, final_height);
            delete gc;
        }
    } else {
        dc.SetTextForeground(*wxLIGHT_GREY);
        dc.DrawLabel(_("Preview"), dc.GetSize(), wxALIGN_CENTER);
    }
}

void ChartController::UpdatePreview() {
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();
    
    std::optional<InMemoryImage> thumb_data_opt = ArtifactFactory::GenerateChartThumbnail(opts, DynaRange::Gui::Constants::CHART_PREVIEW_WIDTH);

    if (thumb_data_opt) {
        const InMemoryImage& thumb_data = *thumb_data_opt;
        // std::memcpy is safe to use for POD types like unsigned char
        unsigned char* data_copy = new unsigned char[thumb_data.data.size()];
        std::memcpy(data_copy, thumb_data.data.data(), thumb_data.data.size());

        wxImage image(thumb_data.width, thumb_data.height, data_copy, false);
        if (image.IsOk()) {
            m_frame->m_chartPreviewBitmap = wxBitmap(image);
        } else {
            wxLogError("Failed to create wxImage for chart preview.");
            m_frame->m_chartPreviewBitmap = wxBitmap();
        }
    } else {
        wxLogError("Failed to generate chart preview thumbnail data.");
        m_frame->m_chartPreviewBitmap = wxBitmap();
    }
    m_frame->m_chartPreviewPanel->Refresh();
}