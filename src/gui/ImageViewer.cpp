// File: gui/ImageViewer.cpp
/**
 * @file gui/ImageViewer.cpp
 * @brief Implements the ImageViewer helper class.
 */
#include "ImageViewer.hpp"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <filesystem>
#include <algorithm> // For std::min

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
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName fn(exePath);
    wxString appDir = fn.GetPath();
    wxString logoPath = appDir + wxFILE_SEP_PATH + "logo.png";
    
    wxString label;
    if (m_originalImage.LoadFile(logoPath, wxBITMAP_TYPE_PNG)) {
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
    if (!m_originalImage.IsOk() || !m_imageControl) {
        return;
    }

    wxSize containerSize = m_imageControl->GetSize();
    if (containerSize.GetWidth() <= 10 || containerSize.GetHeight() <= 10) {
        // If the container has no size yet, we cannot scale.
        // This is normal during initial window creation before layout is finalized.
        return;
    }

    wxImage imageCopy = m_originalImage.Copy();

    int imgWidth = imageCopy.GetWidth();
    int imgHeight = imageCopy.GetHeight();
    double hScale = static_cast<double>(containerSize.GetWidth()) / imgWidth;
    double vScale = static_cast<double>(containerSize.GetHeight()) / imgHeight;
    double scale = std::min(hScale, vScale);

    int newWidth = static_cast<int>(imgWidth * scale);
    int newHeight = static_cast<int>(imgHeight * scale);
    imageCopy.Rescale(newWidth, newHeight, wxIMAGE_QUALITY_HIGH);

    m_imageControl->SetBitmap(wxBitmap(imageCopy));
}