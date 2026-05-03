#ifndef SIMDITOA_SRC_AVX512_CPP
#define SIMDITOA_SRC_AVX512_CPP

// AVX-512 IFMA integer-to-string conversion.
//
// Reference: Champagne Gareau & Lemire, "Converting an Integer to a Decimal
// String in Under Two Nanoseconds" (arXiv:2604.26019v1, 2025).
//
// Algorithm overview (Section 3 of the paper):
//   For each digit position k (1..8), define c_k = floor(2^52 / 10^k).
//   Step 1: low_k = vpmadd52lo(c_k, n, c_k) = (c_k * n + c_k) mod 2^52
//   Step 2: digit_k = vpmadd52hi('0', 10, low_k) = floor(10 * low_k / 2^52) + '0'
//   This extracts the k-th most significant digit as an ASCII character.
//
// For 16-digit values (Section 3.2): split into two 8-digit halves,
// process in parallel, then use vpermi2b (VBMI) to gather digit bytes.
//
// For 17-20 digit values (Section 3.3): peel off 4 trailing digits via
// scalar division, then process the remaining 13-16 digits with the
// 16-digit kernel.
//
// Requires: AVX-512 IFMA (vpmadd52) + VBMI (vpermi2b) instruction sets.
// Available on: Intel Ice Lake+, AMD Zen 4+.

#ifndef SIMDITOA_CONDITIONAL_INCLUDE
  #include "base.h"
#endif

#include "digit_count.h"
#include "digit_utils.h"

#if SIMDITOA_HAS_AVX512_IFMA

  #include <immintrin.h>

namespace simditoa {
namespace avx512 {

// Core 8-digit kernel: extracts all 8 decimal digits of n < 10^8
// into 8 lanes of a __m512i, each containing one ASCII digit value.
//
// The IFMA trick: for each k in [1..8], the constant c_k = floor(2^52 / 10^k).
// vpmadd52lo computes (c_k * n + c_k) mod 2^52 = fractional remainder.
// vpmadd52hi computes floor((low * 10) / 2^52) + '0' = ASCII digit.
simditoa_really_inline __m512i to_string_8digits(uint64_t n) {
  __m512i vn = _mm512_set1_epi64(n);
  constexpr uint64_t twoto52 = 0x10000000000000ULL; // 2^52
  __m512i c =
      _mm512_setr_epi64(twoto52 / 100000000, twoto52 / 10000000,
                        twoto52 / 1000000, twoto52 / 100000, twoto52 / 10000,
                        twoto52 / 1000, twoto52 / 100, twoto52 / 10);
  __m512i vten = _mm512_set1_epi64(10);
  __m512i vzero = _mm512_set1_epi64('0');
  __m512i low = _mm512_madd52lo_epu64(c, vn, c);
  return _mm512_madd52hi_epu64(vzero, vten, low);
}

// 16-digit kernel: extracts all 16 digits of n < 10^16.
// Splits n into high and low 8-digit chunks, processes both in parallel,
// then gathers digits via byte permutation.
simditoa_really_inline __m128i to_string_16digits(uint64_t n) {
  uint64_t n_hi = n / 100000000ULL;
  uint64_t n_lo = n % 100000000ULL;
  __m512i bcstq_h = _mm512_set1_epi64(n_hi);
  __m512i bcstq_l = _mm512_set1_epi64(n_lo);
  constexpr uint64_t twoto52 = 0x10000000000000ULL;
  __m512i c =
      _mm512_setr_epi64(twoto52 / 100000000, twoto52 / 10000000,
                        twoto52 / 1000000, twoto52 / 100000, twoto52 / 10000,
                        twoto52 / 1000, twoto52 / 100, twoto52 / 10);
  __m512i vten = _mm512_set1_epi64(10);
  __m512i vzero = _mm512_set1_epi64('0');

  // Byte permutation mask: picks byte 0 (low byte) from each 8-byte lane
  // across both high and low digit vectors.
  __m512i perm_mask = _mm512_castsi128_si512(
      _mm_set_epi8(0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40, 0x38, 0x30,
                   0x28, 0x20, 0x18, 0x10, 0x08, 0x00));

  __m512i low_h = _mm512_madd52lo_epu64(c, bcstq_h, c);
  __m512i low_l = _mm512_madd52lo_epu64(c, bcstq_l, c);
  __m512i high_h = _mm512_madd52hi_epu64(vzero, vten, low_h);
  __m512i high_l = _mm512_madd52hi_epu64(vzero, vten, low_l);

  // Gather digit bytes: selects byte 0 from each 64-bit lane of both vectors
  __m512i perm = _mm512_permutex2var_epi8(high_h, perm_mask, high_l);
  return _mm512_castsi512_si128(perm);
}

// ---- Conversion routines ----

// Small values: 0 to 99,999,999 (1-8 digits)
simditoa_really_inline int to_chars_small(uint64_t value, char *result) {
  const __m512i digits_7_0 = to_string_8digits(value);
  const int n = fast_digit_count(value);
  const __mmask8 mask = static_cast<__mmask8>(0xffu >> (8 - n));
  // Compress 8 lanes (64-bit each with digit in low byte) to n bytes
  _mm512_mask_cvtusepi64_storeu_epi8(
      result - 8 + n, static_cast<__mmask8>(0xff00u >> n), digits_7_0);
  return n;
}

// Medium values: 10^8 to 10^16-1 (9-16 digits)
simditoa_really_inline int to_chars_medium(uint64_t value, char *result) {
  const __m128i digits_15_0 = to_string_16digits(value);
  const int n = fast_digit_count(value);
  const __mmask16 mask = static_cast<__mmask16>(0xFFFFu << (16 - n));
  _mm_mask_storeu_epi8(result - 16 + n, mask, digits_15_0);
  return n;
}

// Large values: 10^16 to UINT64_MAX (17-20 digits)
simditoa_really_inline int to_chars_large(uint64_t value, char *result) {
  const int n = fast_digit_count(value);
  auto [q, r] = div10000(value);
  const int nq = n - 4;
  const __m128i v16 = to_string_16digits(q);
  const __mmask16 mask = static_cast<__mmask16>(0xFFFFu << (16 - nq));
  _mm_mask_storeu_epi8(result - 16 + nq, mask, v16);
  write_four_digits(result + nq, r);
  return n;
}

// Main dispatcher
simditoa_really_inline size_t to_chars_unsigned(uint64_t value, char *result) {
  if (value < 100000000ULL) {
    return static_cast<size_t>(to_chars_small(value, result));
  }
  if (value < 10000000000000000ULL) {
    return static_cast<size_t>(to_chars_medium(value, result));
  }
  return static_cast<size_t>(to_chars_large(value, result));
}

size_t to_chars(int64_t value, char *buffer) noexcept {
  if (value < 0) {
    *buffer++ = '-';
    uint64_t uval = static_cast<uint64_t>(-(value + 1)) + 1;
    return 1 + to_chars_unsigned(uval, buffer);
  }
  return to_chars_unsigned(static_cast<uint64_t>(value), buffer);
}

size_t to_chars(uint64_t value, char *buffer) noexcept {
  return to_chars_unsigned(value, buffer);
}

} // namespace avx512
} // namespace simditoa

#endif // SIMDITOA_HAS_AVX512_IFMA

#endif // SIMDITOA_SRC_AVX512_CPP
