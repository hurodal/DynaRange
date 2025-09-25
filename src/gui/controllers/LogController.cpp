// File: gui/LogController.cpp
/**
 * @file gui/LogController.cpp
 * @brief Implements the LogController class.
 */
#include "LogController.hpp"
#include <wx/textctrl.h>

LogController::LogController(wxTextCtrl* textCtrl) : m_logCtrl(textCtrl) {}

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