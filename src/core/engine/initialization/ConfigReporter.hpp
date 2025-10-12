// File: src/core/engine/initialization/ConfigReporter.hpp
/**
 * @file ConfigReporter.hpp
 * @brief Declares a component for reporting configuration details to the log.
 * @details This module adheres to SRP by encapsulating all console printing
 * logic for the initialization phase, such as displaying the file pre-analysis
 * table and the final configuration summary.
 */
#pragma once

#include "../../arguments/ArgumentsOptions.hpp"
#include "../../setup/MetadataExtractor.hpp" // For FileInfo
#include <ostream>
#include <vector>

namespace DynaRange::Engine::Initialization {

/**
 * @class ConfigReporter
 * @brief Reports initialization-phase details to the console/log.
 */
class ConfigReporter {
public:
    /**
     * @brief Prints a formatted table of pre-analyzed file information.
     * @param file_info The vector of file metadata.
     * @param log_stream The output stream for logging.
     */
    void PrintPreAnalysisTable(const std::vector<FileInfo>& file_info, std::ostream& log_stream) const;

    /**
     * @brief Prints a summary of the final configuration that will be used for analysis.
     * @param opts The final program options.
     * @param log_stream The output stream for logging.
     */
    void PrintFinalConfiguration(const ProgramOptions& opts, std::ostream& log_stream) const;
};

} // namespace DynaRange::Engine::Initialization