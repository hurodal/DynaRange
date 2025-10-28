// File: src/core/artifacts/image/DebugImageWriter.cpp
/**
 * @file src/core/artifacts/image/DebugImageWriter.cpp
 * @brief Implements the creation and saving logic for debug/auxiliary image artifacts.
 */
#include "DebugImageWriter.hpp"
#include "../../io/OutputWriter.hpp"
#include "../../utils/OutputFilenameGenerator.hpp"
#include <libintl.h>

#define _(string) gettext(string)

namespace { // Anonymous namespace for internal helpers

/**
 * @brief Internal helper to handle filename generation and saving for all Debug/Auxiliary Images.
 * @details This enforces DRY by centralizing the common saving logic for all debug artifacts.
 */
std::optional<fs::path> WriteDebugArtifact(
    const cv::Mat& debug_image,
    const fs::path& filename, // Filename is passed here after generation
    const PathManager& paths,
    std::ostream& log_stream)
{
    if (filename.empty()) {
        log_stream << _("  - Warning: Empty filename provided for debug image.") << std::endl; 
        return std::nullopt;
    }
    // 2. Get full path
    fs::path full_path = paths.GetFullPath(filename);
    // 3. Write file (content is already generated)
    if (OutputWriter::WriteDebugImage(debug_image, full_path, log_stream)) {
        return full_path;
    }
    return std::nullopt;
}

} // end anonymous namespace

namespace ArtifactFactory::Image {

// --- CreatePrintPatchesImage (Movido de ArtifactFactory.cpp) ---
std::optional<fs::path> CreatePrintPatchesImage(
    const cv::Mat& debug_image,
    const OutputNamingContext& ctx,
    const PathManager& paths,
    std::ostream& log_stream)
{
    fs::path filename = OutputFilenameGenerator::GeneratePrintPatchesFilename(ctx);
    return WriteDebugArtifact(debug_image, filename, paths, log_stream);
}

std::optional<fs::path> CreateGenericDebugImage(
    const cv::Mat& debug_image,
    const OutputNamingContext& ctx,
    DebugImageType debug_type,
    const PathManager& paths,
    std::ostream& log_stream)
{
    fs::path filename;
    // Determine the specific filename based on the enum type.
    switch (debug_type) {
        case DebugImageType::PreKeystone:
            filename = OutputFilenameGenerator::GeneratePreKeystoneDebugFilename(ctx);
            break;
        case DebugImageType::PostKeystone:
            filename = OutputFilenameGenerator::GeneratePostKeystoneDebugFilename(ctx);
            break;
        case DebugImageType::CropArea:
            filename = OutputFilenameGenerator::GenerateCropAreaDebugFilename(ctx);
            break;
        case DebugImageType::Corners:
            filename = OutputFilenameGenerator::GenerateCornersDebugFilename(ctx);
            break;        
    }
    // NOTA: La función original CreateGenericDebugImage tomaba el nombre como argumento pre-generado.
    // La nueva función CreateGenericDebugImage aquí genera el nombre internamente.
    // Usamos el helper interno para escribir el archivo.
    return WriteDebugArtifact(debug_image, filename, paths, log_stream);
}

} // namespace ArtifactFactory::Image