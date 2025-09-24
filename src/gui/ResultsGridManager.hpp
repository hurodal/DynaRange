// File: gui/ResultsGridManager.hpp
/**
 * @file gui/ResultsGridManager.hpp
 * @brief Declares a helper class to manage the results grid.
 * @details This class encapsulates all logic for populating the wxGrid
 * from a CSV file, adhering to the Single Responsibility Principle.
 */
#pragma once

#include <wx/grid.h>
#include <string>

class ResultsGridManager {
public:
    /**
     * @brief Constructor.
     * @param gridControl A pointer to the wxGrid this manager will handle.
     */
    explicit ResultsGridManager(wxGrid* gridControl);

    /**
     * @brief Clears the grid and loads new data from a CSV file.
     * @param csv_path The path to the CSV file.
     * @return True on success, false if the file could not be opened.
     */
    bool LoadFromCsv(const std::string& csv_path);

private:
    /**
     * @brief Clears all rows and columns from the grid.
     */
    void ClearGrid();

    /// @brief Pointer to the UI control that displays the data.
    wxGrid* m_gridControl;
};