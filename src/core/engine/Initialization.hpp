// File: src/core/engine/Initialization.hpp
/**
 * @file src/core/engine/Initialization.hpp
 * @brief Declares the function to initialize the analysis environment.
 */
#pragma once
#include "../arguments/ArgumentsOptions.hpp"
#include <ostream>

/**
 * @brief Prepares the analysis environment.
 * @details This function processes dark and saturation frames if provided,
 * prints the final configuration to the log, sorts the input files,
 * and generates a command string for plotting purposes.
 * @param opts A reference to the program options, which will be updated.
 * @param log_stream The output stream for logging messages.
 * @return true if initialization is successful, false otherwise.
 */
bool InitializeAnalysis(ProgramOptions& opts, std::ostream& log_stream);