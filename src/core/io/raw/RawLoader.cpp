// File: src/core/io/raw/RawLoader.cpp
/**
 * @file RawLoader.cpp
 * @brief Implements the RAW file loading component.
 */
#include "RawLoader.hpp"

namespace DynaRange::IO::Raw {

std::shared_ptr<LibRaw> RawLoader::Load(const std::string& filename) {
    auto raw_processor = std::make_shared<LibRaw>();
    if (raw_processor->open_file(filename.c_str()) != LIBRAW_SUCCESS) {
        return nullptr;
    }
    if (raw_processor->unpack() != LIBRAW_SUCCESS) {
        return nullptr;
    }
    return raw_processor;
}

} // namespace DynaRange::IO::Raw