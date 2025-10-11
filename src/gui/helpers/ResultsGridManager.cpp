// File: gui/ResultsGridManager.cpp
/**
 * @file gui/ResultsGridManager.cpp
 * @brief Implements the ResultsGridManager helper class.
 */
#include "ResultsGridManager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm> // For std::count

ResultsGridManager::ResultsGridManager(wxGrid* gridControl) : m_gridControl(gridControl) {}

void ResultsGridManager::ClearGrid() {
    if (m_gridControl->GetNumberRows() > 0) {
        m_gridControl->DeleteRows(0, m_gridControl->GetNumberRows());
    }
    if (m_gridControl->GetNumberCols() > 0) {
        m_gridControl->DeleteCols(0, m_gridControl->GetNumberCols());
    }
}

bool ResultsGridManager::LoadFromCsv(const std::string& csv_path) {
    if (!m_gridControl) return false;

    std::ifstream file(csv_path);
    if (!file) { 
        return false;
    }

    ClearGrid();

    std::string line;
    // Read header
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        int num_cols = std::count(line.begin(), line.end(), ',') + 1;
        m_gridControl->AppendCols(num_cols);
        for(int i = 0; i < num_cols; ++i) m_gridControl->SetColLabelValue(i, "");
        m_gridControl->SetColLabelSize(WXGRID_DEFAULT_COL_LABEL_HEIGHT);
        m_gridControl->AppendRows(1); // Row for header
        int col = 0;
        while (std::getline(ss, cell, ',')) {
            m_gridControl->SetCellValue(0, col, cell);
            m_gridControl->SetReadOnly(0, col++, true);
        }
    }

    // Read data rows
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        m_gridControl->AppendRows(1);
        int grid_row = m_gridControl->GetNumberRows() - 1;
        int col = 0;
        while (std::getline(ss, cell, ',')) {
            m_gridControl->SetCellValue(grid_row, col, cell);
            m_gridControl->SetReadOnly(grid_row, col++, true);
        }
    }

    // Alinear a la derecha todas las columnas excepto la primera (nombres de fichero).
    int num_cols = m_gridControl->GetNumberCols();
    for (int col = 1; col < num_cols; ++col) {
        // Se crea un nuevo atributo para la columna.
        // El grid se hace propietario de este puntero.
        wxGridCellAttr* attr = new wxGridCellAttr();
        attr->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTER); // Alinear a la derecha horizontalmente
        m_gridControl->SetColAttr(col, attr);
    }
    
    // Alinear también las cabeceras de las columnas numéricas.
    if (num_cols > 0) {
        m_gridControl->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTER); // Cabecera del nombre de fichero a la izquierda
        for (int col = 1; col < num_cols; ++col) {
            m_gridControl->SetColLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTER);
        }
    }

    m_gridControl->AutoSize();
    return true;
}