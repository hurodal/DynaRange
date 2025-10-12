// File: src/core/io/raw/RawLoader.hpp
/**
 * @file RawLoader.hpp
 * @brief Declares a component for loading and unpacking RAW files using LibRaw.
 * @details This module adheres to SRP by encapsulating the low-level file
 * access and initial decoding process, separating it from data access and
 * metadata extraction.
 */
#pragma once

#include <libraw/libraw.h>
#include <string>
#include <memory>

namespace DynaRange::IO::Raw {

/**
 * @class RawLoader
 * @brief A static class responsible for loading a RAW file into a LibRaw object.
 */
class RawLoader {
public:
    /**
     * @brief Loads and unpacks a RAW file from a given path.
     * @param filename The path to the RAW file.
     * @return A shared pointer to an initialized LibRaw object on success, or nullptr on failure.
     */
    static std::shared_ptr<LibRaw> Load(const std::string& filename);
};

} // namespace DynaRange::IO::Raw