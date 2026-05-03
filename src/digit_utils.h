#ifndef SIMDITOA_SRC_DIGIT_UTILS_H
#define SIMDITOA_SRC_DIGIT_UTILS_H

// Shared utilities for integer-to-string conversion:
// - Two-digit ASCII lookup tables
// - Fast division helpers using multiplicative inverses
// - 64×64→128 bit multiplication

#include "base.h"
#include <array>
#include <cstring>
#include <utility>

namespace simditoa {

// ---- 64×64→128 bit multiplication ----

struct uint128_t {
  uint64_t high;
  uint64_t low;
};

simditoa_really_inline uint128_t mul64x64(uint64_t a, uint64_t b) {
#if defined(__SIZEOF_INT128__)
  __uint128_t result = static_cast<__uint128_t>(a) * b;
  return {static_cast<uint64_t>(result >> 64), static_cast<uint64_t>(result)};
#elif defined(_MSC_VER) && defined(_M_AMD64)
  uint64_t high;
  uint64_t low = _umul128(a, b, &high);
  return {high, low};
#else
  uint64_t a_lo = a & 0xFFFFFFFF;
  uint64_t a_hi = a >> 32;
  uint64_t b_lo = b & 0xFFFFFFFF;
  uint64_t b_hi = b >> 32;
  uint64_t lo_lo = a_lo * b_lo;
  uint64_t hi_lo = a_hi * b_lo;
  uint64_t lo_hi = a_lo * b_hi;
  uint64_t hi_hi = a_hi * b_hi;
  uint64_t mid = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
  uint64_t high = hi_hi + (hi_lo >> 32) + (mid >> 32);
  uint64_t low = (lo_lo & 0xFFFFFFFF) | (mid << 32);
  return {high, low};
#endif
}

// ---- Division by constants via multiplicative inverses ----

// Division by 100 using a pseudo-remainder for lookup indexing.
// Returns (quotient, pseudo_remainder) where pseudo_remainder is unique per
// x%100.
simditoa_really_inline std::pair<uint64_t, uint64_t> div100v(uint64_t x) {
  uint64_t v = x * uint64_t(0x28f5c29); // ≈ ceil(2^32 / 100)
  return {v >> 32, (v >> 24) & 0xff};
}

// Exact division by 100: returns (quotient, exact remainder).
simditoa_really_inline std::pair<uint64_t, uint64_t> div100(uint64_t x) {
  uint64_t q = mul64x64(x, 0x28f5c28f5c28f5d).high; // ≈ ceil(2^64 / 100)
  return {q, x - 100 * q};
}

// Division by 10000.
simditoa_really_inline std::pair<uint64_t, uint64_t> div10000(uint64_t x) {
  uint64_t q = mul64x64(x, 0x346dc5d63886594bULL).high >> 11;
  return {q, x - 10000 * q};
}

// Division by 10^8.
simditoa_really_inline std::pair<uint64_t, uint64_t> div10e8(uint64_t x) {
  uint64_t q = mul64x64(x, 0xABCC77118461CEFDULL).high >> 26;
  return {q, x - 100000000ULL * q};
}

// Division by 10^16.
simditoa_really_inline std::pair<uint64_t, uint64_t> div10e16(uint64_t x) {
  uint64_t q = mul64x64(x, 0x39A5652FB1137857ULL).high >> 51;
  return {q, x - 10'000'000'000'000'000ULL * q};
}

// ---- Two-digit ASCII lookup tables ----

// Standard table: maps value in [0,99] to its two ASCII digit characters.
inline constexpr std::array<std::array<char, 2>, 100> make_two_digit_table() {
  std::array<std::array<char, 2>, 100> table{};
  for (int i = 0; i < 100; ++i) {
    table[i] = {static_cast<char>('0' + i / 10),
                static_cast<char>('0' + i % 10)};
  }
  return table;
}

inline constexpr auto two_digit_table = make_two_digit_table();

// Pseudo-remainder table: maps div100v pseudo-remainder to two ASCII digits.
// The pseudo-remainder from div100v(x) is unique per x%100, so we build
// a 256-entry table indexed by those pseudo-remainder values.
inline constexpr std::array<std::array<char, 2>, 256> make_two_digit_v_table() {
  std::array<std::array<char, 2>, 256> table{};
  for (int i = 0; i < 10000; ++i) {
    uint64_t v = static_cast<uint64_t>(i) * uint64_t(0x28f5c29);
    uint64_t pseudo = (v >> 24) & 0xff;
    table[pseudo] = {static_cast<char>('0' + (i / 10) % 10),
                     static_cast<char>('0' + i % 10)};
  }
  return table;
}

inline constexpr auto two_digit_v_table = make_two_digit_v_table();

// ---- Digit writing helpers ----

simditoa_really_inline void write_two_digits(char *buf, uint64_t value) {
  std::memcpy(buf, two_digit_table[value].data(), 2);
}

simditoa_really_inline void write_two_digits_v(char *buf, uint64_t pseudo_rem) {
  std::memcpy(buf, two_digit_v_table[pseudo_rem].data(), 2);
}

// Write exactly 4 digits from a value in [0, 9999].
simditoa_really_inline void write_four_digits(char *buf, uint64_t value) {
  auto [high, low] = div100v(value);
  write_two_digits(buf, high);
  write_two_digits_v(buf + 2, low);
}

// Write 1-4 digits from a value in [0, 9999], returns pointer past end.
simditoa_really_inline char *write_1to4_digits(char *buf, uint64_t value) {
  if (value >= 1000) {
    auto [high, low] = div100v(value);
    write_two_digits(buf, high);
    write_two_digits_v(buf + 2, low);
    return buf + 4;
  } else if (value >= 100) {
    auto [high, low] = div100v(value);
    buf[0] = static_cast<char>('0' + high);
    write_two_digits_v(buf + 1, low);
    return buf + 3;
  } else if (value >= 10) {
    write_two_digits(buf, value);
    return buf + 2;
  } else {
    buf[0] = static_cast<char>('0' + value);
    return buf + 1;
  }
}

} // namespace simditoa

#endif // SIMDITOA_SRC_DIGIT_UTILS_H
