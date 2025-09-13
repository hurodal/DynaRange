/**
 * @file core/Engine.hpp
 * @brief Declares the main orchestrator function for the dynamic range analysis.
 */
#pragma once
#include "Arguments.hpp"
#include <ostream>
#include <optional>
#include <string>

/**
 * @brief Runs the complete dynamic range analysis workflow.
 * @details This function orchestrates the three main phases of the analysis:
 * 1. Initialization: Prepares the environment and configuration.
 * 2. Processing: Analyzes all input RAW files.
 * 3. Reporting: Generates the final outputs (plots, CSV, etc.).
 * @param opts Program options. This is passed by reference as it may be modified
 * during initialization (e.g., sorting input files).
 * @param log_stream The stream for logging all output messages.
 * @return An optional string containing the path to the summary plot if generated,
 * otherwise std::nullopt.
 */
std::optional<std::string> RunDynamicRangeAnalysis(ProgramOptions& opts, std::ostream& log_stream);