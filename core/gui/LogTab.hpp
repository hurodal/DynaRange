// core/gui/LogTab.hpp
#pragma once
#include <wx/wx.h>
#include <wx/textctrl.h>

class LogTab : public wxPanel {
public:
    LogTab(wxWindow* parent);
    void ClearLog();
    void AppendLog(const wxString& text);
private:
    wxTextCtrl* m_logText;
};