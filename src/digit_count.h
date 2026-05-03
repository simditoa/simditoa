#ifndef SIMDITOA_SRC_DIGIT_COUNT_H
#define SIMDITOA_SRC_DIGIT_COUNT_H

// Branchless digit counting for 64-bit integers.
// Reference: Daniel Lemire, "Counting the digits of 64-bit integers,"
// https://lemire.me/blog/2025/01/07/counting-the-digits-of-64-bit-integers/

#include "base.h"
#include <cstdint>

namespace simditoa {

simditoa_really_inline int count_leading_zeros_64(uint64_t x) {
  if (x == 0)
    return 64;
#if defined(_MSC_VER)
  unsigned long index;
  _BitScanReverse64(&index, x);
  return 63 - static_cast<int>(index);
#else
  return __builtin_clzll(x);
#endif
}

simditoa_really_inline int fast_digit_count(uint64_t x) {
  static constexpr int digits[65] = {
      19, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16, 16, 15, 15, 15,
      14, 14, 14, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10, 10,
      9,  9,  9,  8,  8,  8,  7,  7,  7,  7,  6,  6,  6,  5,  5,  5,  4,
      4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  1,  1};
  static constexpr uint64_t thresholds[65] = {9999999999999999999ULL,
                                              9999999999999999999ULL,
                                              9999999999999999999ULL,
                                              9999999999999999999ULL,
                                              999999999999999999ULL,
                                              999999999999999999ULL,
                                              999999999999999999ULL,
                                              99999999999999999ULL,
                                              99999999999999999ULL,
                                              99999999999999999ULL,
                                              9999999999999999ULL,
                                              9999999999999999ULL,
                                              9999999999999999ULL,
                                              9999999999999999ULL,
                                              999999999999999ULL,
                                              999999999999999ULL,
                                              999999999999999ULL,
                                              99999999999999ULL,
                                              99999999999999ULL,
                                              99999999999999ULL,
                                              9999999999999ULL,
                                              9999999999999ULL,
                                              9999999999999ULL,
                                              9999999999999ULL,
                                              999999999999ULL,
                                              999999999999ULL,
                                              999999999999ULL,
                                              99999999999ULL,
                                              99999999999ULL,
                                              99999999999ULL,
                                              9999999999ULL,
                                              9999999999ULL,
                                              9999999999ULL,
                                              9999999999ULL,
                                              999999999ULL,
                                              999999999ULL,
                                              999999999ULL,
                                              99999999ULL,
                                              99999999ULL,
                                              99999999ULL,
                                              9999999ULL,
                                              9999999ULL,
                                              9999999ULL,
                                              9999999ULL,
                                              999999ULL,
                                              999999ULL,
                                              999999ULL,
                                              99999ULL,
                                              99999ULL,
                                              99999ULL,
                                              9999ULL,
                                              9999ULL,
                                              9999ULL,
                                              9999ULL,
                                              999ULL,
                                              999ULL,
                                              999ULL,
                                              99ULL,
                                              99ULL,
                                              99ULL,
                                              9ULL,
                                              9ULL,
                                              9ULL,
                                              9ULL,
                                              0ULL};
  int lz = count_leading_zeros_64(x);
  return static_cast<int>(x > thresholds[lz]) + digits[lz];
}

} // namespace simditoa

#endif // SIMDITOA_SRC_DIGIT_COUNT_H
