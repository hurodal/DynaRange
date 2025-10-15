// File: src/core/setup/PreAnalysis.cpp
/**
 * @file src/core/setup/PreAnalysis.cpp
 * @brief Implements the pre-analysis logic for RAW files.
 */
#include "PreAnalysis.hpp"
#include "Constants.hpp"
#include "../io/raw/RawFile.hpp"
#include <opencv2/imgproc.hpp>
#include <libintl.h>

#define _(string) gettext(string)

std::vector<PreAnalysisResult> PreAnalyzeRawFiles(
    const std::vector<std::string>& input_files,
    double saturation_value,
    std::ostream* log_stream)
{
    std::vector<PreAnalysisResult> results;
    results.reserve(input_files.size());
    for (const auto& filename : input_files) {
        RawFile raw_file(filename);
        if (!raw_file.Load()) {
            if (log_stream) {
                (*log_stream) << _("Warning: Could not pre-load RAW file for metadata extraction: ") << filename << std::endl;
            }
            continue;
        }
        cv::Mat active_img = raw_file.GetActiveRawImage();
        if (active_img.empty()) {
            if (log_stream) {
                (*log_stream) << _("[FATAL ERROR] Could not read direct raw sensor data from input file: ") << filename << std::endl;
                (*log_stream) << _("  This is likely because the file is in a compressed RAW format that is not supported for analysis.") << std::endl;
            }
            continue;
        }
        double mean_brightness = cv::mean(active_img)[0];
        
        // Se calcula el ratio de píxeles saturados.
        int saturated_pixels = cv::countNonZero(active_img >= (saturation_value * 0.99));
        double total_pixels = active_img.total();
        double saturation_ratio = (total_pixels > 0) ? static_cast<double>(saturated_pixels) / total_pixels : 0.0;
        
        PreAnalysisResult result;
        result.filename = filename;
        result.mean_brightness = mean_brightness;
        result.iso_speed = raw_file.GetIsoSpeed();
        // Se utiliza la nueva constante para determinar si el fichero está saturado.
        result.has_saturated_pixels = (saturation_ratio > DynaRange::Setup::Constants::MAX_PRE_ANALYSIS_SATURATION_RATIO);

        result.saturation_value_used = saturation_value;
        results.push_back(result);

        if (log_stream) {
            (*log_stream) << _("Pre-analyzed file: ") << filename << std::endl;
        }
    }
    return results;
}