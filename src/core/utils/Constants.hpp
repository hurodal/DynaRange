// File: src/core/utils/Constants.hpp
#pragma once

namespace DynaRange::Utils::Constants {

    // Command-line executable name.
    constexpr const char* CLI_EXECUTABLE_NAME = "rango";

    // --- NEW: Base strings for titles and filenames ---

    // Titles
    constexpr const char* TITLE_BASE_SNR_CURVES = "SNR Curves";
    constexpr const char* TITLE_BASE_SNR_CURVE = "SNR Curve";

    // Filename Prefixes/Bases
    constexpr const char* FNAME_BASE_SNR_CURVE = "snr_curve";
    constexpr const char* FNAME_BASE_SNR_CURVES = "snr_curves";
    constexpr const char* FNAME_BASE_CSV_RESULTS = "results";
    constexpr const char* FNAME_BASE_PRINT_PATCHES = "printpatches";
    constexpr const char* FNAME_BASE_TEST_CHART = "testchart";
    constexpr const char* FNAME_BASE_CORNER_DEBUG = "debug_corners_detected";
    constexpr const char* LOG_OUTPUT_FILENAME = "DynaRange Analysis Results.txt"; // *** NUEVA CONSTANTE *** (Movida desde gui/Constants.hpp)

    // Filename Separators/Components
    constexpr const char* FNAME_SEPARATOR = "_";
    constexpr const char* FNAME_ISO_PREFIX = "ISO";
    constexpr const char* FNAME_AVERAGE_SUFFIX = "average";
    constexpr const char* FNAME_SELECTED_SUFFIX = "selected";

    // File Extensions (including dot)
    constexpr const char* EXT_PNG = ".png";
    constexpr const char* EXT_PDF = ".pdf";
    constexpr const char* EXT_SVG = ".svg";
    constexpr const char* EXT_CSV = ".csv";
    constexpr const char* EXT_TXT = ".txt"; // Added for log file if needed

} // namespace DynaRange::Utils::Constants