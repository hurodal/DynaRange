// File: gui/helpers/ImageViewer.cpp
/**
 * @file gui/helpers/ImageViewer.cpp
 * @brief Implements the ImageViewer helper class.
 */
#include "ImageViewer.hpp"
#include "../../core/utils/PathManager.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <filesystem>
#include <algorithm> // For std::min
#include <wx/dcgraph.h> // For wxGCDC Sharper preview

namespace fs = std::filesystem;

ImageViewer::ImageViewer(wxStaticBitmap* imageControl) : m_imageControl(imageControl) {}

wxString ImageViewer::ShowGraph(const std::string& path) {
    wxString label;
    if (path.empty() || !m_imageControl) {
        return _("Generated Graph:");
    }
    
    fs::path graphPath(path);
    std::string displayFilename = graphPath.filename().string();
    
    if (!fs::exists(graphPath) || !m_originalImage.LoadFile(wxString(graphPath.string()))) {
        m_imageControl->SetBitmap(wxBitmap()); // Clear bitmap on failure
        label = _("Generated Graph (Image not found): ") + wxString(displayFilename);
    } else {
        label = _("Generated Graph: ") + wxString(displayFilename);
        UpdateBitmapDisplay();
    }
    return label;
}

wxString ImageViewer::ShowLogo() {
    // Use PathManager to get the definitive path to the logo asset.
    PathManager path_manager(ProgramOptions{});
    fs::path logo_path = path_manager.GetAssetPath("logo.png");
    
    wxString label;
    if (m_originalImage.LoadFile(logo_path.wstring(), wxBITMAP_TYPE_PNG)) {
        label = _("Welcome to Dynamic Range Calculator");
    } else {
        m_originalImage = wxImage(); // Invalidate the image object on failure
        label = _("Welcome (logo.png not found)");
    }
    UpdateBitmapDisplay();
    return label;
}

void ImageViewer::HandleResize() {
    UpdateBitmapDisplay();
}

void ImageViewer::UpdateBitmapDisplay() {
    if (!m_originalImage.IsOk() || !m_imageControl || !m_imageControl->GetParent()) { 
        return; 
    }

    // Get the size of the parent container, not the image control itself.
    // GetClientSize() gives us the actual usable area within the panel.
    wxSize containerSize = m_imageControl->GetParent()->GetClientSize();
    
    // Allow resizing even with small sizes.
    // Only ignore if the size is zero or negative.
    if (containerSize.GetWidth() <= 0 || containerSize.GetHeight() <= 0) {
        return;
    }

    // If the size is positive, we proceed to scale the image.
    wxImage imageCopy = m_originalImage.Copy();
    int imgWidth = imageCopy.GetWidth();
    int imgHeight = imageCopy.GetHeight();

    double hScale = static_cast<double>(containerSize.GetWidth()) / imgWidth;
    double vScale = static_cast<double>(containerSize.GetHeight()) / imgHeight;
    double scale = std::min(hScale, vScale);

    imageCopy.Rescale(imgWidth * scale, imgHeight * scale, wxIMAGE_QUALITY_HIGH);

    m_imageControl->SetBitmap(wxBitmap(imageCopy));
    
    if (m_imageControl->GetParent()) {
        m_imageControl->GetParent()->Layout();
    }
}

void ImageViewer::SetImage(const wxImage& image)
{
    if (!m_imageControl) {
        return;
    }

    if (!image.IsOk()) {
        m_originalImage = wxImage(); // Invalidate the image
        m_imageControl->SetBitmap(wxBitmap()); // Clear the control
    } else {
        m_originalImage = image.Copy(); // Store the new image
        UpdateBitmapDisplay(); // Scale and display it
    }
}