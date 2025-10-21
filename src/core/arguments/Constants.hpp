// File: src/core/arguments/Constants.hpp
/**
 * @file src/core/arguments/Constants.hpp
 * @brief Centralizes all command-line argument name constants.
 * @details Using constants for argument names avoids magic strings and ensures
 * consistency across the application. A change here propagates everywhere.
 */
#pragma once

namespace DynaRange::Arguments::Constants {

    // --- Core Analysis Arguments ---
    constexpr const char* BlackLevel = "black-level";
    constexpr const char* BlackFile = "black-file";
    constexpr const char* SaturationLevel = "saturation-level";
    constexpr const char* SaturationFile = "saturation-file";
    constexpr const char* InputFiles = "input-files";
    constexpr const char* PatchRatio = "patch-ratio";
    constexpr const char* SnrThresholdDb = "snrthreshold-db";
    constexpr const char* DrNormalizationMpx = "drnormalization-mpx";
    constexpr const char* RawChannels = "raw-channels";
    constexpr const char* PolyFit = "poly-fit";

    // --- Output and Plotting Arguments ---
    constexpr const char* OutputFile = "output-file";
    constexpr const char* PlotFormat = "plot-format";
    constexpr const char* PlotParams = "plot-params";
    constexpr const char* PrintPatches = "print-patches";

    // --- Chart Generation Arguments ---
    constexpr const char* Chart = "chart";
    constexpr const char* ChartColour = "chart-colour";
    constexpr const char* ChartPatches = "chart-patches";
    constexpr const char* ChartCoords = "chart-coords";

    // --- Internal Flags (no user-facing CLI equivalent) ---
    constexpr const char* GeneratePlot = "generate-plot";
    constexpr const char* CreateChartMode = "create-chart-mode";
    constexpr const char* SnrThresholdIsDefault = "snr-threshold-is-default"; // Still needed by parser logic
    constexpr const char* BlackLevelIsDefault = "black-level-is-default";
    constexpr const char* SaturationLevelIsDefault = "saturation-level-is-default";

    /** @brief Internal name for storing the manual camera name from the GUI. */
    constexpr const char* GuiManualCameraName = "gui-manual-camera-name";
    /** @brief Internal name for storing the 'Use EXIF Name' flag state from the GUI. */
    constexpr const char* GuiUseExifNameFlag = "gui-use-exif-flag";
    /** @brief Internal name for storing the 'Add Suffix' flag state from the GUI. */
    constexpr const char* GuiUseSuffixFlag = "gui-use-suffix-flag";

    // Debug plotting
    constexpr const char* FullDebug = "debug"; // Argumento para activar debug extendido en runtime
} // namespace DynaRange::Arguments::Constants