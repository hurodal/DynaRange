// File: core/utils/PathManager.hpp
/**
 * @file core/utils/PathManager.hpp
 * @brief Declares a utility class for managing output file paths.
 */
#pragma once

#include "../arguments/ProgramOptions.hpp"
#include "../analysis/Analysis.hpp"
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @class PathManager
 * @brief Centralizes the logic for creating all output file and directory paths.
 * @details This class has the single responsibility of generating paths for reports,
 * plots, and other outputs, ensuring consistent naming conventions.
 */
class PathManager {
public:
    /**
     * @brief Constructs a PathManager.
     * @details It intelligently determines the base output directory. If the output
     * filename in opts is a simple filename without a path, it prepends the user's
     * Documents directory. Otherwise, it respects the full or relative path provided.
     * @param opts The program options, used to determine base paths.
     */
    explicit PathManager(const ProgramOptions& opts);

    /**
     * @brief Gets the full path for the main CSV output file.
     * @return The filesystem path to the CSV file.
     */
    fs::path GetCsvOutputPath() const;

    /**
     * @brief Gets the full path for an individual SNR plot PNG.
     * @param curve The data for the curve, used to generate a dynamic filename.
     * @return The filesystem path for the individual plot.
     */
    fs::path GetIndividualPlotPath(const CurveData& curve) const;

    /**
     * @brief Gets the full path for the summary SNR plot PNG.
     * @param camera_name The name of the camera, used in the filename.
     * @return The filesystem path for the summary plot.
     */
    fs::path GetSummaryPlotPath(const std::string& camera_name) const;

private:
    fs::path m_output_directory; ///< The base directory for all outputs.
    fs::path m_csv_filename;     ///< The filename for the CSV report.
};