// File: src/core/artifacts/data/ReportWriter.cpp
/**
 * @file src/core/artifacts/data/ReportWriter.cpp
 * @brief Implements functions for generating and saving data and log file artifacts.
 */
#include "ReportWriter.hpp"
#include "../../utils/OutputFilenameGenerator.hpp"
#include "../../utils/Formatters.hpp"
#include "../../utils/Constants.hpp"
#include "../../io/OutputWriter.hpp"
#include <fstream>
#include <libintl.h>

#define _(string) gettext(string)

namespace ArtifactFactory::Report {

std::optional<fs::path> CreateCsvReport(
    const std::vector<DynamicRangeResult>& results,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream)
{
    // 1. Generate filename
    fs::path filename = OutputFilenameGenerator::GenerateCsvFilename(ctx);
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Format data
    auto sorted_rows = Formatters::FlattenAndSortResults(results);
    // 4. Write file
    if (OutputWriter::WriteCsv(sorted_rows, full_path, log_stream)) {
        return full_path;
    }
    return std::nullopt;
}

std::optional<fs::path> CreateLogFile(
    const std::string& log_content,
    const OutputNamingContext& ctx,
    const fs::path& base_output_directory)
{
    // 1. Generate filename (base + suffix + extension)
    // Use constant from core/utils/Constants.hpp
    std::string filename_str = std::string(DynaRange::Utils::Constants::LOG_OUTPUT_FILENAME);
    // Add camera suffix if present (using the now public GetSafeCameraSuffix)
    filename_str.insert(filename_str.find_last_of('.'), OutputFilenameGenerator::GetSafeCameraSuffix(ctx));

    fs::path filename = filename_str;
    // 2. Get full path
    fs::path full_path = base_output_directory / filename;
    // 3. Write file
    std::ofstream log_file_stream(full_path);
    if (log_file_stream.is_open()) {
        log_file_stream << log_content;
        log_file_stream.close();

        // No logging here, caller should log success/failure
        return full_path;
    } else {
        // No logging here, caller should log success/failure
        return std::nullopt;
    }
}

} // namespace ArtifactFactory::Report