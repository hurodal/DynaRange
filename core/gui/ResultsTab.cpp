// core/gui/ResultsTab.cpp
#include "ResultsTab.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/intl.h>
#include <fstream>
#include <sstream>
#include <string>

ResultsTab::ResultsTab(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    m_csvGrid = new wxGrid(this, wxID_ANY);
    m_csvGrid->CreateGrid(0, 0);
    m_imageCtrl = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(this, wxID_ANY, _("CSV Output:")), 0, wxALL, 5);
    sizer->Add(m_csvGrid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(new wxStaticText(this, wxID_ANY, _("Generated Graph (placeholder):")), 0, wxALL, 5);
    sizer->Add(m_imageCtrl, 1, wxEXPAND | wxALL, 5);
    this->SetSizer(sizer);
}

void ResultsTab::LoadResults(const ProgramOptions& opts) {
    std::ifstream file(opts.output_filename);
    if (file) {
        std::string line;
        int row = 0;
        if (m_csvGrid->GetNumberRows() > 0) m_csvGrid->DeleteRows(0, m_csvGrid->GetNumberRows());
        if (m_csvGrid->GetNumberCols() > 0) m_csvGrid->DeleteCols(0, m_csvGrid->GetNumberCols());
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string cell;
            int col = 0;
            m_csvGrid->AppendRows(1);
            while (std::getline(ss, cell, ',')) {
                if (col >= m_csvGrid->GetNumberCols()) { m_csvGrid->AppendCols(1); }
                m_csvGrid->SetCellValue(row, col, cell);
                col++;
            }
            row++;
        }
        m_csvGrid->AutoSize();
        this->Layout();
    }
}