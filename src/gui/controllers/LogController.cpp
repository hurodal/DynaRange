// File: gui/LogController.cpp
/**
 * @file gui/LogController.cpp
 * @brief Implements the LogController class.
 */
#include "LogController.hpp"
#include <wx/textctrl.h>
#include <wx/font.h>

LogController::LogController(wxTextCtrl* textCtrl) : m_logCtrl(textCtrl) {
    // Set a cross-platform monospace font to ensure log alignment.
    // wxFONTFAMILY_MODERN is a generic family for fixed-width fonts.
    if (m_logCtrl) {
        wxFont font = wxFontInfo(10).Family(wxFONTFAMILY_MODERN);
        m_logCtrl->SetFont(font);
    }
}

void LogController::AppendText(const wxString& text) {
    if (m_logCtrl) {
        m_logCtrl->AppendText(text);
    }
}

void LogController::Clear() {
    if (m_logCtrl) {
        m_logCtrl->Clear();
    }
}