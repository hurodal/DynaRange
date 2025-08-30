// core/gui/ResultsTab.hpp
#pragma once
#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/statbmp.h>
#include "../../core/arguments.hpp"

class ResultsTab : public wxPanel {
public:
    ResultsTab(wxWindow* parent);
    void LoadResults(const ProgramOptions& opts);
private:
    wxGrid* m_csvGrid;
    wxStaticBitmap* m_imageCtrl;
};