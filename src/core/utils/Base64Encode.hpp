// File: src/core/utils/Base64Encode.hpp
/**
 * @file src/core/utils/Base64Encode.hpp
 * @brief Implements the data formatting utility functions.
 */

#pragma once

#include <string>

namespace Base64Encode {

/**
 * @brief Encodes a block of binary data into a Base64 string.
 * @param data Pointer to the input data.
 * @param len The length of the input data in bytes.
 * @return The Base64 encoded string.
 */
std::string base64_encode(const unsigned char* data, size_t len);	
} // End namespace Base64Encode