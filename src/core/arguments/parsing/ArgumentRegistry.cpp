// File: src/core/arguments/parsing/ArgumentRegistry.cpp
/**
 * @file ArgumentRegistry.cpp
 * @brief Implements the argument registry for the application.
 */
#include "ArgumentRegistry.hpp"
#include "../Constants.hpp"
#include "../ArgumentsOptions.hpp"
#include <libintl.h>

#define _(string) gettext(string)

namespace DynaRange::Arguments::Parsing {

std::map<std::string, ArgumentDescriptor> ArgumentRegistry::RegisterAll()
{
    std::map<std::string, ArgumentDescriptor> descriptors;
    using namespace DynaRange::Arguments::Constants;

    descriptors[BlackLevel] = { BlackLevel, "B", _("Camera RAW black level"), ArgType::Double, DEFAULT_BLACK_LEVEL };
    descriptors[BlackFile] = { BlackFile, "b", _("Totally dark RAW file ideally shot at base ISO"), ArgType::String, std::string("") };
    descriptors[SaturationLevel] = { SaturationLevel, "S", _("Camera RAW saturation level"), ArgType::Double, DEFAULT_SATURATION_LEVEL };
    descriptors[SaturationFile] = { SaturationFile, "s", _("Totally clipped RAW file ideally shot at base ISO"), ArgType::String, std::string("") };
    descriptors[InputFiles] = { InputFiles, "i", _("Input RAW files shot over the test chart ideally for every ISO"), ArgType::StringVector, std::vector<std::string> {}, false };
    descriptors[PatchRatio] = { PatchRatio, "r", _("Relative patch width/height used to compute signal and noise readings (default=0.5)"), ArgType::Double, DEFAULT_PATCH_RATIO, false, 0.0, 1.0 };
    descriptors[SnrThresholdDb] = { SnrThresholdDb, "d", _("SNR threshold(s) list in dB for DR calculation (default=0 12 being 0dB=\"Engineering DR\" and 12dB=\"Photographic DR\")"), ArgType::DoubleVector, DEFAULT_SNR_THRESHOLDS_DB };    
    descriptors[DrNormalizationMpx] = { DrNormalizationMpx, "m", _("Number of Mpx for DR normalization (default=8Mpx, no normalization=per pixel DR=0Mpx)"), ArgType::Double, DEFAULT_DR_NORMALIZATION_MPX };
    descriptors[PolyFit] = { PolyFit, "f", _("Polynomic order to fit the SNR curve (default=3)"), ArgType::Int, DEFAULT_POLY_ORDER, false, 2, 3 };
    descriptors[OutputFile] = { OutputFile, "o", _("Output CSV text file(s) with all results..."), ArgType::String, std::string(DEFAULT_OUTPUT_FILENAME) };
    descriptors[PlotFormat] = { PlotFormat, "p", _("Export SNR curves plot in PNG/SVG format (default format=PNG)"), ArgType::String, std::string("PNG") };
    descriptors[PlotParams] = { PlotParams, "P", _("Export SNR curves with SCL 1-3 info (default=1 1 1 3)"), ArgType::IntVector, std::vector<int> { 1, 1, 1, 3 } };
    std::string print_patches_help = std::string(_("Save keystone/ETTR/gamma corrected test chart in PNG format indicating the grid of patches used for all calculations (default=\"")) + DEFAULT_PRINT_PATCHES_FILENAME + "\")";
    descriptors[PrintPatches] = { PrintPatches, "g", print_patches_help, ArgType::String, std::string("") };
    descriptors[RawChannels] = { RawChannels, "w", _("Specify with 0/1 boolean values for which RAW channel(s) the calculations (SNR curves, DR) will be carried out (default=0 0 0 0 1)"), ArgType::IntVector, std::vector<int> { 0, 0, 0, 0, 1 } };
    descriptors[GeneratePlot] = { GeneratePlot, "", "", ArgType::Flag, false };

    // Chart arguments
    descriptors[Chart] = { Chart, "c", _("specify format of test chart (default DIMX=1920, W=3, H=2)"), ArgType::IntVector, std::vector<int>() };
    descriptors[ChartColour] = { ChartColour, "C", _("Create test chart in PNG format ranging colours..."), ArgType::StringVector, std::vector<std::string>() };
    descriptors[ChartCoords] = { ChartCoords, "x", _("Test chart defined by 4 corners: tl, bl, br, tr"), ArgType::DoubleVector, std::vector<double>() };
    descriptors[ChartPatches] = { ChartPatches, "M", _("Specify number of patches over rows (M) and columns (N) (default M=4, N=6)"), ArgType::IntVector, std::vector<int>() };

    // Internal flags
    descriptors[CreateChartMode] = { CreateChartMode, "", "", ArgType::Flag, false };
    descriptors[SnrThresholdIsDefault] = { SnrThresholdIsDefault, "", "", ArgType::Flag, true };
    descriptors[BlackLevelIsDefault] = { BlackLevelIsDefault, "", "", ArgType::Flag, true };
    descriptors[SaturationLevelIsDefault] = { SaturationLevelIsDefault, "", "", ArgType::Flag, true };
    
    return descriptors;
}
} // namespace DynaRange::Arguments::Parsing