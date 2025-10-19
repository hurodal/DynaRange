// File: src/gui/Constants.hpp
/**
 * @file src/gui/Constants.hpp
 * @brief Centralizes constants related to the Graphical User Interface.
 */
#pragma once
#include <vector>
#include <string>
#include <wx/string.h>
#include <wx/intl.h> // Needed for wxTRANSLATE

namespace DynaRange::Gui::Constants {

    /**
     * @brief The width in pixels for the generated chart preview thumbnail.
     */
    constexpr int CHART_PREVIEW_WIDTH = 1024;

    /**
     * @brief Scaling factor applied to the base plot dimensions for GUI rendering.
     * @details A factor of 0.75 on a 1920x1080 base results in a 1440x810 image,
     * which provides a good balance of quality and performance for the preview.
     */
    constexpr double GUI_RENDER_SCALE_FACTOR = 0.75;

    // *** CONSTANTE LOG_OUTPUT_FILENAME ELIMINADA DE AQUÍ ***
    // (Movida a src/core/utils/Constants.hpp)
    // constexpr const char* LOG_OUTPUT_FILENAME = "DynaRange Analysis Results.txt";

    /**
     * @brief The default initial value for the gamma/contrast slider (0-100).
     */
    constexpr int DEFAULT_GAMMA_SLIDER_VALUE = 90;

    /**
     * @brief Fallback list of supported RAW file extensions.
     */
    const std::vector<std::string> FALLBACK_RAW_EXTENSIONS = {
        "3fr", "ari", "arw", "bay", "crw", "cr2", "cr3", "cap", "data", "dcs",
        "dcr", "dng", "drf", "eip", "erf", "fff", "gpr", "iiq", "k25", "kdc",
        "mdc", "mef", "mos", "mrw", "nef", "nrw", "obm", "orf", "pef", "ptx",
        "pxn", "r3d", "raf", "raw", "rwl", "rw2", "rwz", "sr2", "srf", "srw", "x3f"
    };
    /**
     * @brief Generates the wildcard filter string for file dialogs.
     * @param extensions The list of file extensions (without dots).
     * @return A wxString formatted for use in wxFileDialog.
     */
    inline wxString GetSupportedExtensionsWildcard(const std::vector<std::string>& extensions)
    {
        wxString wildcard;
        for (size_t i = 0; i < extensions.size(); ++i) {
            wxString ext_lower = extensions[i];
            wxString ext_upper = ext_lower.Upper();
            wildcard += wxString::Format("*.%s;*.%s", ext_lower, ext_upper);
            if (i < extensions.size() - 1) {
                wildcard += ";";
            }
        }
        return wxString::Format(
            // Use wxTRANSLATE to mark strings for translation
            wxTRANSLATE("RAW files (%s)|%s|All files (*.*)|*.*"),
            wildcard, wildcard
        );
    }

    /**
     * @namespace AvgChoices
     * @brief Defines the available options and indices for the Average Mode wxChoice.
     */
    namespace AvgChoices {
        // Mark strings for translation using wxTRANSLATE
        const wxString No = wxTRANSLATE("No");
        const wxString Full = wxTRANSLATE("Full");
        const wxString Selected = wxTRANSLATE("Only Selected");

        constexpr int IDX_NO = 0;
        constexpr int IDX_FULL = 1;
        constexpr int IDX_SELECTED = 2;
        // Index if it were present

        const wxString DEFAULT_CHOICE = Full;
        // Default selection string
        constexpr int DEFAULT_INDEX = IDX_FULL;
        // Default selection index
    }

} // namespace DynaRange::Gui::Constants