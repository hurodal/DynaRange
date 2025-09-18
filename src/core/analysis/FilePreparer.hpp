// File: src/core/analysis/FilePreparer.hpp
/**
 * @file src/core/analysis/FilePreparer.hpp
 * @brief Declares the function to pre-analyze and sort input files by brightness or ISO.
 */
#pragma once

#include "../arguments/ProgramOptions.hpp"
#include <ostream>

/**
 * @brief Pre-analyzes input files to sort them by brightness or ISO.
 * @details This ensures that files are processed in order of exposure, which can
 * be important for some analyses. The file list within 'opts' is modified in place.
 * @param opts Program options, passed by reference to modify the input file list.
 * @param log_stream Stream for logging messages.
 * @return true if successful, false if no files could be processed.
 */
bool PrepareAndSortFiles(ProgramOptions& opts, std::ostream& log_stream);