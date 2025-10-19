// File: src/core/arguments/parsing/ArgumentRegistry.cpp
/**
 * @file ArgumentRegistry.cpp
 * @brief Implements the argument registry for the application.
 */
#include "ArgumentRegistry.hpp"
#include "../Constants.hpp"
#include "../ArgumentsOptions.hpp" // For default values and constants
#include <libintl.h>
#include <string> 
#include <vector> 
#include <map>    

#define _(string) gettext(string)

namespace DynaRange::Arguments::Parsing {

/**
 * @brief Registers and returns descriptors for all application arguments.
 * @details Defines CLI arguments, chart arguments, and internal flags including those for GUI state.
 * @return A map where the key is the argument's long name and the value is its descriptor.
 */
std::map<std::string, ArgumentDescriptor> ArgumentRegistry::RegisterAll()
{
    std::map<std::string, ArgumentDescriptor> descriptors;
    using namespace DynaRange::Arguments::Constants;

    // --- Core Analysis Arguments ---
    descriptors[BlackLevel] = { BlackLevel, "B", _("Camera RAW black level"), ArgType::Double, DEFAULT_BLACK_LEVEL };
    descriptors[BlackFile] = { BlackFile, "b", _("Totally dark RAW file ideally shot at base ISO"), ArgType::String, std::string("") };
    descriptors[SaturationLevel] = { SaturationLevel, "S", _("Camera RAW saturation level"), ArgType::Double, DEFAULT_SATURATION_LEVEL };
    descriptors[SaturationFile] = { SaturationFile, "s", _("Totally clipped RAW file ideally shot at base ISO"), ArgType::String, std::string("") };
    descriptors[InputFiles] = { InputFiles, "i", _("Input RAW files shot over the test chart ideally for every ISO"), ArgType::StringVector, std::vector<std::string> {}, false }; // is_required=false, handled by CLI parser logic
    descriptors[PatchRatio] = { PatchRatio, "r", _("Relative patch width/height used to compute signal and noise readings (default=0.5)"), ArgType::Double, DEFAULT_PATCH_RATIO, false, 0.0, 1.0 };
    descriptors[SnrThresholdDb] = { SnrThresholdDb, "d", _("SNR threshold(s) list in dB for DR calculation (default=12 0)"), ArgType::DoubleVector, DEFAULT_SNR_THRESHOLDS_DB };
    descriptors[DrNormalizationMpx] = { DrNormalizationMpx, "m", _("Number of Mpx for DR normalization (default=8Mpx, no normalization=per pixel DR=0Mpx)"), ArgType::Double, DEFAULT_DR_NORMALIZATION_MPX };
    descriptors[PolyFit] = { PolyFit, "f", _("Polynomic order to fit the SNR curve (default=3)"), ArgType::Int, DEFAULT_POLY_ORDER, false, 2, 3 }; // Min/Max values specified
    descriptors[RawChannels] = { RawChannels, "w", _("Specify flags (R G1 G2 B) and mode (AVG: 0=No, 1=Full, 2=Selected) for analysis (default=0 0 0 0 1)"), ArgType::IntVector, std::vector<int> { 0, 0, 0, 0, 1 } };

    // --- Output and Plotting Arguments ---
    descriptors[OutputFile] = { OutputFile, "o", _("Output CSV text file with all results"), ArgType::String, std::string(DEFAULT_OUTPUT_FILENAME) };
    descriptors[PlotFormat] = { PlotFormat, "p", _("Export SNR curves plot format (PNG, PDF, SVG)"), ArgType::String, std::string("PNG") };
    descriptors[PlotParams] = { PlotParams, "P", _("Plot elements (S C L) and command mode (1-3): Scatters Curve Labels Cmd (default=1 1 1 3)"), ArgType::IntVector, std::vector<int> { 1, 1, 1, 3 } };
    std::string print_patches_help = std::string(_("Save debug image showing patches used (default=\"")) + DEFAULT_PRINT_PATCHES_FILENAME + "\")";
    descriptors[PrintPatches] = { PrintPatches, "g", print_patches_help, ArgType::String, std::string("_USE_DEFAULT_PRINT_PATCHES_") }; // Use sentinel default

    // --- Chart Generation Arguments ---
    descriptors[Chart] = { Chart, "c", _("Generate chart: DIMX W H [M N] (def=1920 3 2 [4 6])"), ArgType::IntVector, std::vector<int>() };
    descriptors[ChartColour] = { ChartColour, "C", _("Generate chart: R G B [InvGamma] (def=255 101 164 [1.4])"), ArgType::StringVector, std::vector<std::string>() };
    // Initialize default chart patches using constants from ArgumentsOptions.hpp
    descriptors[ChartPatches] = { ChartPatches, "M", _("Patches grid: M Rows, N Cols (def=4 6)"), ArgType::IntVector, std::vector<int>{DEFAULT_CHART_PATCHES_M, DEFAULT_CHART_PATCHES_N} };
    descriptors[ChartCoords] = { ChartCoords, "x", _("Manual chart corners: x1 y1 x2 y2 x3 y3 x4 y4"), ArgType::DoubleVector, std::vector<double>() };

    // --- Internal Flags (no CLI exposure) ---
    descriptors[GeneratePlot] = { GeneratePlot, "", "", ArgType::Flag, false };
    descriptors[CreateChartMode] = { CreateChartMode, "", "", ArgType::Flag, false };
    descriptors[SnrThresholdIsDefault] = { SnrThresholdIsDefault, "", "", ArgType::Flag, true };
    descriptors[BlackLevelIsDefault] = { BlackLevelIsDefault, "", "", ArgType::Flag, true };
    descriptors[SaturationLevelIsDefault] = { SaturationLevelIsDefault, "", "", ArgType::Flag, true };

    // --- NEW INTERNAL FLAGS FOR GUI STATE ---
    // These have no short name (-) or help text as they aren't exposed via CLI.
    // Default values match those defined in ProgramOptions.
    descriptors[GuiManualCameraName] = { GuiManualCameraName, "", "", ArgType::String, std::string("") }; // Default empty string
    // Use defaults from ProgramOptions (defined in ArgumentsOptions.hpp) to respect DRY
    ProgramOptions default_opts;
    descriptors[GuiUseExifNameFlag] = { GuiUseExifNameFlag, "", "", ArgType::Flag, default_opts.gui_use_exif_camera_name };
    descriptors[GuiUseSuffixFlag] = { GuiUseSuffixFlag, "", "", ArgType::Flag, default_opts.gui_use_camera_suffix };

    return descriptors;
}
} // namespace DynaRange::Arguments::Parsing