// File: gui/LogController.hpp
/**
 * @file gui/LogController.hpp
 * @brief Declares a controller class for the LogPanel's logic.
 */
#pragma once

class wxTextCtrl;
class wxString;

class LogController {
public:
    explicit LogController(wxTextCtrl* textCtrl);
    void AppendText(const wxString& text);
    void Clear();
private:
    wxTextCtrl* m_logCtrl;
};