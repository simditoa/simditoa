#ifndef SIMDITOA_H
#define SIMDITOA_H

/**
 * @file simditoa.h
 * @brief SIMD-accelerated integer-to-string conversion.
 *
 * Usage:
 *   #include "simditoa.h"
 *
 *   char buf[simditoa::MAX_INT64_CHARS + 1];
 *   size_t len = simditoa::to_chars(12345, buf);
 *   buf[len] = '\0'; // null-terminate if needed
 */

#include "simditoa/base.h"

#endif // SIMDITOA_H
