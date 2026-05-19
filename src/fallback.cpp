#ifndef SIMDITOA_SRC_FALLBACK_CPP
#define SIMDITOA_SRC_FALLBACK_CPP

#ifndef SIMDITOA_CONDITIONAL_INCLUDE
  #include "base.h"
#endif

#include "digit_count.h"
#include "digit_utils.h"

namespace simditoa {
namespace fallback {

// Improved scalar fallback using two-digit lookup tables.
// Strategy: precompute digit count, then write digit pairs from most
// significant to least significant using multiplicative division, avoiding
// reversal.
size_t to_chars_unsigned(uint64_t value, char *buffer) noexcept {
  if (simditoa_unlikely(value == 0)) {
    buffer[0] = '0';
    return 1;
  }

  int n = fast_digit_count(value);
  char *p = buffer + n;

  // Process 4 digits at a time from the end
  while (value >= 10000) {
    auto [q, r] = div10000(value);
    value = q;
    p -= 4;
    write_four_digits(p, r);
  }

  // Handle remaining 1-4 digits
  if (value >= 100) {
    auto [q, r] = div100(value);
    p -= 2;
    write_two_digits(p, r);
    if (q >= 10) {
      p -= 2;
      write_two_digits(p, q);
    } else {
      p[-1] = static_cast<char>('0' + q);
    }
  } else if (value >= 10) {
    p -= 2;
    write_two_digits(p, value);
  } else {
    p[-1] = static_cast<char>('0' + value);
  }

  return static_cast<size_t>(n);
}

size_t to_chars(int64_t value, char *buffer) noexcept {
  uint64_t uval;
  if (value < 0) {
    *buffer++ = '-';
    uval = static_cast<uint64_t>(-(value + 1)) + 1;
    return 1 + to_chars_unsigned(uval, buffer);
  }
  uval = static_cast<uint64_t>(value);
  return to_chars_unsigned(uval, buffer);
}

size_t to_chars(uint64_t value, char *buffer) noexcept {
  return to_chars_unsigned(value, buffer);
}

// The scalar fallback has no separate homogeneous/heterogeneous paths:
// the variant distinction only changes code generation for the SIMD
// kernel. Both names delegate to the same routine here so callers can
// use a uniform API across implementations.

size_t to_chars_heterogeneous(uint64_t value, char *buffer) noexcept {
  return to_chars(value, buffer);
}
size_t to_chars_heterogeneous(int64_t value, char *buffer) noexcept {
  return to_chars(value, buffer);
}
size_t to_chars_homogeneous(uint64_t value, char *buffer) noexcept {
  return to_chars(value, buffer);
}
size_t to_chars_homogeneous(int64_t value, char *buffer) noexcept {
  return to_chars(value, buffer);
}

} // namespace fallback
} // namespace simditoa

#endif // SIMDITOA_SRC_FALLBACK_CPP
