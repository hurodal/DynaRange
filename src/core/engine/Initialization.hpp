// File: src/core/engine/Initialization.hpp
/**
 * @file src/core/engine/Initialization.hpp
 * @brief Declares the function to initialize the analysis environment.
 */
#pragma once
#include "../arguments/ArgumentsOptions.hpp"
#include "../io/raw/RawFile.hpp"
#include <ostream>
#include <map>
#include <string>
#include <vector>

/**
 * @struct InitializationResult
 * @brief Holds all the data produced by the initialization phase.
 * @details This decouples the initialization logic from the main orchestrator by
 * providing a clear, explicit contract for its outputs.
 */
struct InitializationResult {
    bool success = false;
    std::vector<RawFile> loaded_raw_files;
    std::vector<std::string> sorted_filenames;
    std::map<std::string, std::string> plot_labels;
    double sensor_resolution_mpx = 0.0;
    std::string generated_command;
    double dark_value = 0.0;
    double saturation_value = 0.0;
    bool black_level_is_default = true;
    bool saturation_level_is_default = true;
    int source_image_index = 0;
    std::string bayer_pattern;
};

/**
 * @brief Prepares the analysis environment.
 * @details This function processes dark/saturation frames, sorts input files, etc.
 * It no longer modifies its input parameters; instead, it returns all its
 * findings in the InitializationResult struct.
 * @param opts The program options, used as read-only input for the initialization process.
 * @param log_stream The output stream for logging messages.
 * @return An InitializationResult struct containing all calculated values and state.
 */
InitializationResult InitializeAnalysis(const ProgramOptions& opts, std::ostream& log_stream);