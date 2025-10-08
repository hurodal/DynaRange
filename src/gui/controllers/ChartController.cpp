// File: src/gui/controllers/ChartController.cpp
/**
 * @file gui/controllers/ChartController.cpp
 * @brief Implements the ChartController class.
 */
#include "ChartController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../helpers/WebViewUtils.hpp"
#include "../Constants.hpp" 
#include "../../core/arguments/ChartOptionsParser.hpp" // For the struct
#include "../../core/graphics/ChartGenerator.hpp"
#include "../../core/utils/PathManager.hpp"
#include "../utils/Base64Encode.hpp"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/mstream.h>

ChartController::ChartController(DynaRangeFrame* frame) : m_frame(frame)
{
    // --- 1. Bind resize event for the Chart tab ---
    m_frame->rightColChartPanel->Bind(wxEVT_SIZE, &ChartController::OnRightPanelSize, this);

    // --- 2. Create the wxWebView inside the placeholder panel from the UI designer ---
    m_frame->m_chartPreviewWebView = wxWebView::New(m_frame->m_webView2PlaceholderPanel, wxID_ANY);
    m_frame->m_chartPreviewWebView->Bind(wxEVT_WEBVIEW_ERROR, &DynaRangeFrame::OnWebViewError, m_frame);

    // --- 3. Create a new sizer for the placeholder panel and add the webview to it ---
    wxBoxSizer* placeholderSizer = new wxBoxSizer(wxVERTICAL);
    placeholderSizer->Add(m_frame->m_chartPreviewWebView, 1, wxEXPAND, 0);
    m_frame->m_webView2PlaceholderPanel->SetSizer(placeholderSizer);
    
    // --- 4. Layout the parent panels to make the changes visible ---
    m_frame->rightColChartPanel->Layout();

    // --- 5. Set initial values in the UI controls ---
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
        m_frame->rightColChartPanel->Unbind(wxEVT_SIZE, &ChartController::OnRightPanelSize, this);
    }
}

void ChartController::OnPreviewClick(wxCommandEvent& event) {
    // 1. Get current options from the UI.
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();

    // 2. Generate the thumbnail image data in memory.
    std::optional<InMemoryImage> thumb_data_opt = GenerateChartThumbnail(opts, DynaRange::Gui::Constants::CHART_PREVIEW_WIDTH);

    if (thumb_data_opt) {
        const InMemoryImage& thumb_data = *thumb_data_opt;
        
        // 3. Convert the raw RGB data to a wxImage.
        // The 'true' flag tells wxImage not to take ownership of the data buffer.
        wxImage image(thumb_data.width, thumb_data.height, const_cast<unsigned char*>(thumb_data.data.data()), true);
        
        // 4. Save the wxImage as a PNG to a memory stream.
        wxMemoryOutputStream memory_stream;
        if (image.SaveFile(memory_stream, wxBITMAP_TYPE_PNG)) {
            // 5. Correctly read the generated PNG data from the beginning of the stream.
            size_t size = memory_stream.GetSize();
            std::vector<unsigned char> png_buffer(size);
            
            // Create a memory *input* stream to read from the output stream's buffer.
            wxMemoryInputStream input_stream(memory_stream);
            input_stream.Read(png_buffer.data(), size); // Read all data from the start.

            // 6. Base64-encode the PNG data.
            std::string encoded_data = Base64Encode::base64_encode(png_buffer.data(), png_buffer.size());
            wxString data_uri = "data:image/png;base64," + encoded_data;

            // 7. Generate and render the responsive HTML.
            std::string html = WebViewUtils::CreateHtmlForImage(data_uri);
            m_frame->m_chartPreviewWebView->SetPage(html, "");
            
        } else {
            wxLogError("Failed to save chart preview to memory stream.");
            m_frame->m_chartPreviewWebView->LoadURL("about:blank");
        }
    } else {
        wxLogError("Failed to generate chart preview thumbnail.");
        m_frame->m_chartPreviewWebView->LoadURL("about:blank");
    }
}

void ChartController::OnCreateClick(wxCommandEvent& event) {
    ChartGeneratorOptions opts = GetCurrentOptionsFromUi();
    // Use a temporary ProgramOptions to leverage PathManager
    ProgramOptions temp_prog_opts; 
    PathManager paths(temp_prog_opts);
    fs::path chart_output_path = paths.GetCsvOutputPath().parent_path() / "magentachart_gui.png";
    if (GenerateTestChart(opts, chart_output_path.string(), std::cout)) {
        wxMessageBox(wxString::Format(_("Chart saved successfully to:%s"), chart_output_path.string()), _("Success"), wxOK | wxICON_INFORMATION);
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
    // Force the layout of the parent panel to ensure children are redrawn correctly.
    if (m_frame && m_frame->rightColChartPanel) {
        m_frame->rightColChartPanel->Layout();
    }
    event.Skip();
}