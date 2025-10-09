// File: src/core/io/OutputWriter.cpp
/**
 * @file src/core/io/OutputWriter.cpp
 * @brief Implements the OutputWriter module.
 */
#include "OutputWriter.hpp"
#include "../utils/Formatters.hpp"
#include <fstream>
#include <libintl.h>
#include <opencv2/imgcodecs.hpp>

#define _(string) gettext(string)

namespace OutputWriter {

// This function existed previously and has been modified.
bool WritePng(cairo_surface_t* surface, const fs::path& path, std::ostream& log_stream) {
    if (!surface) return false;
    // Use path.string().c_str() for cross-platform compatibility.
    // This converts the path to a std::string (using char) before getting the C-style string,
    // which resolves the wchar_t* vs char* conflict on Windows.
    cairo_status_t status = cairo_surface_write_to_png(surface, path.string().c_str());
    if (status == CAIRO_STATUS_SUCCESS) {
        log_stream << _("  - Info: Plot saved to: ") << path.string() << std::endl;
        return true;
    }
    return false;
}

bool WriteCsv(
    const std::vector<DynamicRangeResult>& all_results,
    const ProgramOptions& opts,
    const fs::path& path,
    std::ostream& log_stream)
{
    std::ofstream csv_file(path);
    if (!csv_file.is_open()) {
        log_stream << "\n" << _("Error: Could not open CSV file for writing: ") << path.string() << std::endl;
        return false;
    }

    // Write the new, fixed header
    csv_file << Formatters::FormatCsvHeader() << std::endl;
    
    // Iterate through results and write the multiple rows that each may generate
    for (const auto& res : all_results) {
        csv_file << Formatters::FormatCsvRows(res);
    }

    csv_file.close();
    log_stream << "\n" << _("Results saved to ") << path.string() << std::endl;
    return true;
}

bool WriteDebugImage(const cv::Mat& image, const fs::path& path, std::ostream& log_stream)
{
    if (image.empty()) {
        return false;
    }
    try {
        // Convert the 32-bit float image (range 0.0-1.0) to an 8-bit unsigned
        // integer image (range 0-255) for saving as a standard PNG.
        cv::Mat output_image;
        image.convertTo(output_image, CV_8U, 255.0);
        
        // Estas líneas estaban comentadas. Al descomentarlas, se activará el guardado.
        if (cv::imwrite(path.string(), output_image)) {
            log_stream << _("  - Info: Debug patch image saved to: ") << path.string() << std::endl;
            return true;
        }
    } catch (const cv::Exception& e) {
        log_stream << _("Error: OpenCV exception while saving debug image: ") << e.what() << std::endl;
    }
    return false;
}
} // namespace OutputWriter