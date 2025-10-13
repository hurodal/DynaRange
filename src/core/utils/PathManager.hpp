// File: src/core/utils/PathManager.hpp
/**
 * @file core/utils/PathManager.hpp
 * @brief Declares a utility class for managing output file paths.
 */
#pragma once

#include "../arguments/ArgumentsOptions.hpp"
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
     * @param opts The program options, used to determine base paths.
     */
    explicit PathManager(const ProgramOptions& opts);

    /**
     * @brief Gets the full path for the main CSV output file.
     * @return The filesystem path to the CSV file.
     */
    fs::path GetCsvOutputPath() const;

    /**
     * @brief Gets the full path for an individual SNR plot.
     * @param curve The data for the curve, used to generate a dynamic filename.
     * @param channels The selection state of the RAW channels for the suffix.
     * @param format The plot output format for the file extension.
     * @return The filesystem path for the individual plot.
     */
    fs::path GetIndividualPlotPath(const CurveData& curve, const RawChannelSelection& channels, DynaRange::Graphics::Constants::PlotOutputFormat format) const;

    /**
     * @brief Gets the full path for the summary SNR plot.
     * @param camera_name The name of the camera, used in the filename.
     * @param channels The selection state of the RAW channels for the suffix.
     * @param format The plot output format for the file extension.
     * @return The filesystem path for the summary plot.
     */
    fs::path GetSummaryPlotPath(const std::string& camera_name, const RawChannelSelection& channels, DynaRange::Graphics::Constants::PlotOutputFormat format) const;

    /**
     * @brief Gets the path to the application's executable directory.
     * @return The filesystem path to the directory containing the running executable.
     */
    fs::path GetAppDirectory() const;

    /**
     * @brief Gets the path to the 'locale' directory for internationalization.
     * @return The filesystem path to the locale directory.
     */
    fs::path GetLocaleDirectory() const;

    /**
     * @brief Gets the full path for a given asset file (e.g., "logo.png").
     * @param asset_name The filename of the asset.
     * @return The full filesystem path to the asset.
     */
    fs::path GetAssetPath(const std::string& asset_name) const;

private:
    
    fs::path m_app_directory;      ///< The directory where the application executable resides.
    fs::path m_output_directory;   ///< The base directory for all outputs.
    fs::path m_csv_filename;       ///< The filename for the CSV report.
};