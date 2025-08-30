// core/gui/LogTab.cpp
#include "LogTab.hpp"
#include <wx/sizer.h>

LogTab::LogTab(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    m_logText = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_logText, 1, wxEXPAND);
    this->SetSizer(sizer);
}

void LogTab::ClearLog() {
    m_logText->Clear();
}

void LogTab::AppendLog(const wxString& text) {
    m_logText->AppendText(text);
}