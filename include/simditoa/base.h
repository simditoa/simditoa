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
 * Uses the heterogeneous variant by default, matching the prior behavior.
 *
 * @param value  The integer to convert.
 * @param buffer Output buffer. Must have at least MAX_DIGITS + 1 bytes.
 * @return       The number of characters written (not null-terminated).
 */
size_t to_chars(int64_t value, char *buffer) noexcept;

/**
 * Convert an unsigned 64-bit integer to its decimal string representation.
 *
 * Uses the heterogeneous variant by default, matching the prior behavior.
 *
 * @param value  The integer to convert.
 * @param buffer Output buffer. Must have at least MAX_DIGITS + 1 bytes.
 * @return       The number of characters written (not null-terminated).
 */
size_t to_chars(uint64_t value, char *buffer) noexcept;

// =====================================================================
// Variant-tagged single-integer API
// =====================================================================
//
// Both variants produce identical output. The choice only affects
// performance on a batch of conversions, where one variant suits the
// input distribution better than the other. See Variant below.

/**
 * Heterogeneous variant (paper §5.4): branch-light, masked stores.
 *
 * Best on inputs whose digit lengths vary unpredictably. Performance is
 * uniform across digit lengths because the same control path runs for
 * every value.
 */
size_t to_chars_heterogeneous(int64_t value, char *buffer) noexcept;
size_t to_chars_heterogeneous(uint64_t value, char *buffer) noexcept;

/**
 * Homogeneous variant (paper §5.5): per-length dispatcher, unmasked
 * direct stores.
 *
 * Best when most inputs in a batch share the same digit length: the
 * dispatcher's branches are well-predicted and each path is tight
 * straight-line code. ~10-12% fewer instructions per digit than the
 * heterogeneous variant on uniform workloads.
 *
 * Writes up to 16 bytes past the start of the buffer even for shorter
 * inputs (the trailing bytes hold zeros). The buffer contract of
 * MAX_DIGITS + 1 bytes covers this.
 */
size_t to_chars_homogeneous(int64_t value, char *buffer) noexcept;
size_t to_chars_homogeneous(uint64_t value, char *buffer) noexcept;

// =====================================================================
// Batch API with dynamic variant selection (paper §5.6, Algorithm 1)
// =====================================================================

/// Variant tag for batch conversion.
enum class Variant : int {
  Heterogeneous = 0,
  Homogeneous = 1,
};

/// Tuning parameters for the dynamic variant-selection step.
struct BatchOptions {
  /// Fraction of the input examined when choosing a variant. Range (0, 1].
  /// Default 0.01 (1%) — overhead is negligible (<0.07% of conversion time
  /// in the paper's measurements).
  double sampling_rate = 0.01;
  /// If the most common digit length appears in at least this fraction of
  /// the sample, select the homogeneous variant. Range (0, 1].
  /// Default 0.95.
  double homogeneity_threshold = 0.95;
};

/**
 * Sample the input and decide which variant best fits its digit-length
 * distribution. Runs in Theta(sampling_rate * count) time with O(1)
 * memory.
 */
Variant select_variant(const uint64_t *values, size_t count,
                       const BatchOptions &opts = BatchOptions{}) noexcept;

/**
 * Convert a batch of unsigned integers using the specified variant.
 *
 * @param values       Input values.
 * @param count        Number of values.
 * @param output_ptrs  Per-element output buffer pointers. Each buffer
 *                     must have at least MAX_DIGITS + 1 bytes.
 * @param lengths      Per-element output: digits written for values[i].
 * @param variant      Which variant to use for every element.
 */
void to_chars_batch(const uint64_t *values, size_t count,
                    char *const *output_ptrs, size_t *lengths,
                    Variant variant) noexcept;

/**
 * Convert a batch of unsigned integers, choosing the variant
 * automatically via select_variant().
 */
void to_chars_batch(const uint64_t *values, size_t count,
                    char *const *output_ptrs, size_t *lengths,
                    const BatchOptions &opts = BatchOptions{}) noexcept;

} // namespace simditoa

#endif // SIMDITOA_BASE_H
