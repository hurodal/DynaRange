// File: src/core/engine/Initialization.hpp
/**
 * @file src/core/engine/Initialization.hpp
 * @brief Declares the function to initialize the analysis environment.
 */
#pragma once
#include "../arguments/ArgumentsOptions.hpp"
#include "../io/raw/RawFile.hpp"
#include <ostream>

/**
 * @brief Prepares the analysis environment.
 * @details This function processes dark/saturation frames, sorts input files, etc.
 * @param opts A reference to the program options, which will be updated.
 * @param log_stream The output stream for logging messages.
 * @return A pair containing a boolean for success and the vector of loaded RawFile objects.
 */
std::pair<bool, std::vector<RawFile>> InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream);