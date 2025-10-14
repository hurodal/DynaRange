// File: src/gui/controllers/ManualCoordsController.cpp
/**
 * @file ManualCoordsController.cpp
 * @brief Implements the ManualCoordsController class.
 */
#include "ManualCoordsController.hpp"
#include "../DynaRangeFrame.hpp"
#include "../GuiPresenter.hpp"
#include "../Constants.hpp"
#include "../../core/io/raw/RawFile.hpp"
#include "../helpers/CvWxImageConverter.hpp"
#include "../helpers/RawExtensionHelper.hpp"
#include <libraw/libraw.h> // For dynamic extension check
#include <opencv2/imgproc.hpp>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/graphics.h>
#include <wx/log.h>

// Helper function from InputController, needed here as well
namespace {
const std::vector<std::string>& GetSupportedRawExtensions() {
    static std::vector<std::string> extensions;
    if (extensions.empty()) {
        #if defined(LIBRAW_MAJOR_VERSION) && defined(LIBRAW_MINOR_VERSION)
            #if LIBRAW_MAJOR_VERSION > 0 || (LIBRAW_MAJOR_VERSION == 0 && LIBRAW_MINOR_VERSION >= 22)
                LibRaw proc;
                int count = 0;
                const char** ext_list = proc.get_supported_extensions_list(&count);
                if (ext_list) {
                    std::set<std::string> unique;
                    for (int i = 0; i < count; ++i) {
                        if (!ext_list[i]) continue;
                        std::string ext(ext_list[i]);
                        if (!ext.empty() && ext[0] == '.') { ext = ext.substr(1); }
                        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });
                        if (!ext.empty()) { unique.insert(ext); }
                    }
                    extensions.assign(unique.begin(), unique.end());
                }
            #endif
        #endif

        if (extensions.empty()) {
            extensions = DynaRange::Gui::Constants::FALLBACK_RAW_EXTENSIONS;
        }
    }
    return extensions;
}
}

ManualCoordsController::ManualCoordsController(DynaRangeFrame* frame) : m_frame(frame)
{
    // Bind events for this tab
    m_frame->m_RawCoordsFilePicker->Bind(wxEVT_FILEPICKER_CHANGED, &ManualCoordsController::OnRawFileChanged, this);
    m_frame->m_clearAllCoordinates->Bind(wxEVT_BUTTON, &ManualCoordsController::OnClearAllClick, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_PAINT, &ManualCoordsController::OnPaint, this);
    m_frame->m_rawImagePreviewPanel->Bind(wxEVT_SIZE, &ManualCoordsController::OnSize, this);
}

ManualCoordsController::~ManualCoordsController() = default;

void ManualCoordsController::OnClearAllClick(wxCommandEvent& event)
{
    m_frame->m_coordX1Value->Clear();
    m_frame->m_coordY1Value->Clear();
    m_frame->m_coordX2Value->Clear();
    m_frame->m_coordY2Value->Clear();
    m_frame->m_coordX3Value->Clear();
    m_frame->m_coordY3Value->Clear();
    m_frame->m_coordX4Value->Clear();
    m_frame->m_coordY4Value->Clear();
    m_frame->m_presenter->UpdateCommandPreview();
}

void ManualCoordsController::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(m_frame->m_rawImagePreviewPanel);
    dc.Clear(); // Clear background

    if (m_rawPreviewImage.IsOk()) {
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            double img_w = m_rawPreviewImage.GetWidth();
            double img_h = m_rawPreviewImage.GetHeight();
            const wxSize panel_size = dc.GetSize();
            
            double scale_factor = std::min((double)panel_size.GetWidth() / img_w, (double)panel_size.GetHeight() / img_h);
            double final_width = img_w * scale_factor;
            double final_height = img_h * scale_factor;
            double offset_x = (panel_size.GetWidth() - final_width) / 2.0;
            double offset_y = (panel_size.GetHeight() - final_height) / 2.0;
            
            wxImage displayImage = m_rawPreviewImage.Copy();
            displayImage.Rescale(final_width, final_height, wxIMAGE_QUALITY_HIGH);
            wxBitmap bitmapToDraw(displayImage);

            gc->DrawBitmap(bitmapToDraw, offset_x, offset_y, final_width, final_height);
            delete gc;
        }
    }
}

void ManualCoordsController::OnSize(wxSizeEvent& event)
{
    m_frame->m_rawImagePreviewPanel->Refresh();
    event.Skip();
}

void ManualCoordsController::LoadSourceImage()
{
    const auto& opts = m_frame->m_presenter->GetLastRunOptions();
    if (opts.source_image_index >= 0 && opts.source_image_index < m_frame->GetInputFiles().size())
    {
        std::string source_file = m_frame->GetInputFiles()[opts.source_image_index];
        DisplayRawFile(source_file);
    } else if (!m_frame->GetInputFiles().empty()) {
        // Fallback to the first file if the index is somehow invalid
        DisplayRawFile(m_frame->GetInputFiles()[0]);
    } else {
        DisplayRawFile(""); // Clear the display if no files are loaded
    }
}

void ManualCoordsController::DisplayRawFile(const std::string& path)
{
    if (path.empty()) {
        m_rawPreviewImage = wxImage(); // Clear image
        m_originalRawWidth = 0;
        m_originalRawHeight = 0;
    } else {
        RawFile raw_file(path);
        if (raw_file.Load()) {
            cv::Mat full_res_mat = raw_file.GetProcessedImage();
            if (full_res_mat.empty()) {
                m_rawPreviewImage = wxImage();
                wxLogError("Could not get processed image from RAW file: %s", path);
            } else {
                // Store original dimensions for future coordinate mapping
                m_originalRawWidth = full_res_mat.cols;
                m_originalRawHeight = full_res_mat.rows;

                // Define a maximum dimension for the preview
                constexpr int MAX_PREVIEW_DIMENSION = 1920;
                cv::Mat preview_mat;

                // Only resize if the image is larger than the max preview dimension
                if (m_originalRawWidth > MAX_PREVIEW_DIMENSION || m_originalRawHeight > MAX_PREVIEW_DIMENSION) {
                    double scale = static_cast<double>(MAX_PREVIEW_DIMENSION) / std::max(m_originalRawWidth, m_originalRawHeight);
                    cv::resize(full_res_mat, preview_mat, cv::Size(), scale, scale, cv::INTER_AREA);
                } else {
                    preview_mat = full_res_mat;
                }
                
                m_rawPreviewImage = GuiHelpers::CvMatToWxImage(preview_mat);
            }
        } else {
            m_rawPreviewImage = wxImage(); // Clear on failure
            m_originalRawWidth = 0;
            m_originalRawHeight = 0;
            wxLogError("Could not load RAW file for preview: %s", path);
        }
    }
    m_frame->m_rawImagePreviewPanel->Refresh();
}

void ManualCoordsController::OnRawFileChanged(wxFileDirPickerEvent& event)
{
    DisplayRawFile(event.GetPath().ToStdString());
    // The event must be skipped to allow the native control to process it.
    event.Skip();
}

std::vector<double> ManualCoordsController::GetChartCoords() const
{
    std::vector<double> coords;
    coords.reserve(8);
    // List of all coordinate text controls
    std::vector<wxTextCtrl*> controls = {
        m_frame->m_coordX1Value, m_frame->m_coordY1Value,
        m_frame->m_coordX2Value, m_frame->m_coordY2Value,
        m_frame->m_coordX3Value, m_frame->m_coordY3Value,
        m_frame->m_coordX4Value, m_frame->m_coordY4Value
    };
    for (wxTextCtrl* control : controls) {
        wxString value_str = control->GetValue();
        // If any field is empty, we consider the whole set invalid.
        if (value_str.IsEmpty()) {
            return {}; // Return empty vector
        }
        double val;
        if (!value_str.ToDouble(&val)) {
            // If conversion fails for any field, also return empty.
            return {};
        }
        coords.push_back(val);
    }

    return coords;
}