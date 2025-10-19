// File: src/core/utils/OutputNamingContext.hpp
/**
 * @file src/core/utils/OutputNamingContext.hpp
 * @brief Defines a context structure holding data for generating output names and titles.
 * @details Aggregates information needed by PlotTitleGenerator and OutputFilenameGenerator,
 * receiving the final effective camera name decision from the calling layer (GUI or CLI).
 */
#pragma once

#include "../arguments/ArgumentsOptions.hpp" // For RawChannelSelection
#include "../graphics/Constants.hpp"       // For PlotOutputFormat enum definition
#include <string>
#include <optional> 
/**
 * @struct OutputNamingContext
 * @brief Holds necessary data for generating plot titles and output filenames.
 * @details The calling layer (GUI or CLI) is responsible for determining the
 * effective_camera_name_for_output based on its own logic (e.g., GUI checkboxes, CLI defaults).
 */
struct OutputNamingContext {
    // --- Source Data (Relevant for naming/titles) ---
    /** @brief Camera model extracted from EXIF metadata (can be used as reference). */
    std::string camera_name_exif;
    /** @brief ISO speed for the specific file (used for individual plots/titles). Optional. */
    std::optional<float> iso_speed;
    /** @brief User's selection of RAW channels to analyze/average. */
    RawChannelSelection raw_channels;
    /** @brief Requested plot output format (PNG, PDF, SVG). Defaults to PNG. */
    DynaRange::Graphics::Constants::PlotOutputFormat plot_format = DynaRange::Graphics::Constants::PlotOutputFormat::PNG;
    /** @brief Custom CSV filename from --output-file argument (if provided). Empty otherwise. */
    std::string user_csv_filename;
    /** @brief Custom debug patches filename from --print-patches argument (if provided). Empty otherwise.
     * @details May contain the sentinel value "_USE_DEFAULT_PRINT_PATCHES_" internally. */
    std::string user_print_patches_filename;

    // --- Decision made by the calling layer (GUI/CLI) ---
    /** @brief The final camera name string to use for output suffixes/titles.
     * @details This is determined by the GUI based on its checkboxes and text field,
     * or set to empty/EXIF name by the CLI. If empty, no camera suffix is added. */
    std::string effective_camera_name_for_output = "";
};