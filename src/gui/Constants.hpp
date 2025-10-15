// File: src/gui/Constants.hpp
// File: src/gui/Constants.hpp
/**
 * @file src/gui/Constants.hpp
 * @brief Centralizes constants related to the Graphical User Interface.
 */
#pragma once
#include <vector>
#include <string>
#include <wx/string.h>
#include <wx/intl.h>

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

    /**
     * @brief The default filename for the log output when saving is enabled.
     */
    constexpr const char* LOG_OUTPUT_FILENAME = "DynaRange Analysis Results.txt";

    /**
     * @brief (New) Fallback list of supported RAW file extensions.
     */
    const std::vector<std::string> FALLBACK_RAW_EXTENSIONS = {
        "3fr", "ari", "arw", "bay", "crw", "cr2", "cr3", "cap", "data", "dcs",
        "dcr", "dng", "drf", "eip", "erf", "fff", "gpr", "iiq", "k25", "kdc",
        "mdc", "mef", "mos", "mrw", "nef", "nrw", "obm", "orf", "pef", "ptx",
        "pxn", "r3d", "raf", "raw", "rwl", "rw2", "rwz", "sr2", "srf", "srw", "x3f"
    };

    /**
     * @brief (New Function) Generates the wildcard filter string for file dialogs.
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
            _("RAW files (%s)|%s|All files (*.*)|*.*"),
            wildcard, wildcard
        );
    }

} // namespace DynaRange::Gui::Constants