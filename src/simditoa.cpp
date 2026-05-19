#define SIMDITOA_SRC_SIMDITOA_CPP

#include "base.h"
#include "digit_count.h"

#include <cmath>

SIMDITOA_PUSH_DISABLE_UNUSED_WARNINGS

// Fallback is always compiled; other implementations may delegate to it.
#include "fallback.cpp"

#define SIMDITOA_CONDITIONAL_INCLUDE

#if SIMDITOA_IMPLEMENTATION_AVX512 && SIMDITOA_HAS_AVX512_IFMA
  #include "avx512.cpp"
#endif

#undef SIMDITOA_CONDITIONAL_INCLUDE

namespace simditoa {

// ---- Single-integer public API ----

size_t to_chars(int64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars(value, buffer);
}

size_t to_chars(uint64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars(value, buffer);
}

size_t to_chars_heterogeneous(int64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars_heterogeneous(value, buffer);
}

size_t to_chars_heterogeneous(uint64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars_heterogeneous(value, buffer);
}

size_t to_chars_homogeneous(int64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars_homogeneous(value, buffer);
}

size_t to_chars_homogeneous(uint64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars_homogeneous(value, buffer);
}

// ---- Dynamic variant selection (paper §5.6, Algorithm 1) ----

Variant select_variant(const uint64_t *values, size_t count,
                       const BatchOptions &opts) noexcept {
  if (count == 0) {
    return Variant::Heterogeneous;
  }

  // m = ceil(sampling_rate * count), clamped to [1, count].
  double rate = opts.sampling_rate;
  if (!(rate > 0.0)) {
    rate = 0.01;
  }
  if (rate > 1.0) {
    rate = 1.0;
  }
  size_t m =
      static_cast<size_t>(std::ceil(rate * static_cast<double>(count)));
  if (m == 0) {
    m = 1;
  }
  if (m > count) {
    m = count;
  }

  // Digit counts run from 1 to 20 inclusive for 64-bit unsigned values.
  int counts[21] = {0};

  if (m == count) {
    // Small input: just scan everything.
    for (size_t i = 0; i < count; ++i) {
      int len = fast_digit_count(values[i]);
      if (len < 1) {
        len = 1;
      }
      if (len > 20) {
        len = 20;
      }
      ++counts[len];
    }
  } else {
    // Pseudo-random sample via xorshift64*, seeded deterministically from
    // count so the same input always produces the same selection. A plain
    // stride sample aliases on periodic inputs (e.g., alternating digit
    // lengths defeat any even stride).
    uint64_t state = 0x9E3779B97F4A7C15ULL ^ static_cast<uint64_t>(count);
    for (size_t i = 0; i < m; ++i) {
      state ^= state >> 12;
      state ^= state << 25;
      state ^= state >> 27;
      uint64_t r = state * 0x2545F4914F6CDD1DULL;
      size_t idx = static_cast<size_t>(r % static_cast<uint64_t>(count));
      int len = fast_digit_count(values[idx]);
      if (len < 1) {
        len = 1;
      }
      if (len > 20) {
        len = 20;
      }
      ++counts[len];
    }
  }
  const size_t actual = m;

  int cmax = 0;
  for (int i = 1; i <= 20; ++i) {
    if (counts[i] > cmax) {
      cmax = counts[i];
    }
  }

  const double rho_max =
      static_cast<double>(cmax) / static_cast<double>(actual);
  return (rho_max >= opts.homogeneity_threshold) ? Variant::Homogeneous
                                                 : Variant::Heterogeneous;
}

// ---- Batch conversion ----

void to_chars_batch(const uint64_t *values, size_t count,
                    char *const *output_ptrs, size_t *lengths,
                    Variant variant) noexcept {
  if (variant == Variant::Homogeneous) {
    for (size_t i = 0; i < count; ++i) {
      lengths[i] = SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars_homogeneous(
          values[i], output_ptrs[i]);
    }
  } else {
    for (size_t i = 0; i < count; ++i) {
      lengths[i] = SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars_heterogeneous(
          values[i], output_ptrs[i]);
    }
  }
}

void to_chars_batch(const uint64_t *values, size_t count,
                    char *const *output_ptrs, size_t *lengths,
                    const BatchOptions &opts) noexcept {
  const Variant v = select_variant(values, count, opts);
  to_chars_batch(values, count, output_ptrs, lengths, v);
}

} // namespace simditoa

SIMDITOA_POP_DISABLE_UNUSED_WARNINGS
