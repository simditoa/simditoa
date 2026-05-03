#ifndef SIMDITOA_BASE_H
#define SIMDITOA_BASE_H

#include "simditoa/common_defs.h"
#include "simditoa/implementation_detection.h"
#include "simditoa/simditoa_version.h"

#include <cstddef>
#include <cstdint>

namespace simditoa {

/// Maximum decimal digits for a 64-bit unsigned integer: "18446744073709551615"
/// = 20 chars. For signed: "-9223372036854775808" = 20 chars. Callers should
/// provide at least 21 bytes of buffer space.
constexpr size_t MAX_DIGITS = 20;

/**
 * Convert a signed 64-bit integer to its decimal string representation.
 *
 * @param value  The integer to convert.
 * @param buffer Output buffer. Must have at least MAX_DIGITS + 1 bytes.
 * @return       The number of characters written (not null-terminated).
 */
size_t to_chars(int64_t value, char *buffer) noexcept;

/**
 * Convert an unsigned 64-bit integer to its decimal string representation.
 *
 * @param value  The integer to convert.
 * @param buffer Output buffer. Must have at least MAX_DIGITS + 1 bytes.
 * @return       The number of characters written (not null-terminated).
 */
size_t to_chars(uint64_t value, char *buffer) noexcept;

} // namespace simditoa

#endif // SIMDITOA_BASE_H
