#ifndef SIMDITOA_SRC_AVX512_CPP
#define SIMDITOA_SRC_AVX512_CPP

// AVX-512 IFMA integer-to-string conversion.
//
// Reference: Champagne Gareau & Lemire, "Converting an Integer to a Decimal
// String in Under Two Nanoseconds" (arXiv:2604.26019v1, 2026).
//
// The kernel (§5.2) replaces repeated division with parallel multiplicative
// remainders computed via AVX-512 IFMA's vpmadd52lo/vpmadd52hi instructions.
// Two complete conversion routines are built on top:
//
//   - Heterogeneous variant (§5.4): branch-light, uses masked stores to
//     handle variable-length output uniformly. Best when digit lengths in
//     a batch vary unpredictably.
//
//   - Homogeneous variant (§5.5): per-length dispatcher with direct
//     unmasked stores. Branches are well-predicted when most inputs in a
//     batch share the same digit length, yielding fewer instructions and
//     ~10-12% fewer cycles per digit on uniform workloads.
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

// ---- Shared SIMD kernels ----

// Core 8-digit kernel: extracts all 8 decimal digits of n < 10^8
// into 8 lanes of a __m512i, each containing one ASCII digit value in the
// low byte of its lane.
//
// The IFMA trick: for each k in [1..8], c_k = floor(2^52 / 10^k).
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

// 16-digit kernel: extracts all 16 digits of n < 10^16 into a __m128i,
// with byte 0 = most significant digit, byte 15 = least significant.
// Splits n into high/low 8-digit chunks, processes both in parallel,
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

  __m512i perm_mask = _mm512_castsi128_si512(
      _mm_set_epi8(0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40, 0x38, 0x30,
                   0x28, 0x20, 0x18, 0x10, 0x08, 0x00));

  __m512i low_h = _mm512_madd52lo_epu64(c, bcstq_h, c);
  __m512i low_l = _mm512_madd52lo_epu64(c, bcstq_l, c);
  __m512i high_h = _mm512_madd52hi_epu64(vzero, vten, low_h);
  __m512i high_l = _mm512_madd52hi_epu64(vzero, vten, low_l);

  __m512i perm = _mm512_permutex2var_epi8(high_h, perm_mask, high_l);
  return _mm512_castsi512_si128(perm);
}

// =====================================================================
// Heterogeneous variant (§5.4)
// =====================================================================
//
// Branch-light: a single 3-way dispatch (small/medium/large) followed by
// a masked store. The mask is computed from the digit count at runtime,
// avoiding per-length branches. Best on heterogeneous workloads where
// digit lengths vary unpredictably.

// 1-8 digits.
simditoa_really_inline int het_to_chars_small(uint64_t value, char *result) {
  const __m512i digits_7_0 = to_string_8digits(value);
  const int n = fast_digit_count(value);
  _mm512_mask_cvtusepi64_storeu_epi8(
      result - 8 + n, static_cast<__mmask8>(0xff00u >> n), digits_7_0);
  return n;
}

// 9-16 digits.
simditoa_really_inline int het_to_chars_medium(uint64_t value, char *result) {
  const __m128i digits_15_0 = to_string_16digits(value);
  const int n = fast_digit_count(value);
  const __mmask16 mask = static_cast<__mmask16>(0xFFFFu << (16 - n));
  _mm_mask_storeu_epi8(result - 16 + n, mask, digits_15_0);
  return n;
}

// 17-20 digits.
simditoa_really_inline int het_to_chars_large(uint64_t value, char *result) {
  const int n = fast_digit_count(value);
  auto [q, r] = div10000(value);
  const int nq = n - 4;
  const __m128i v16 = to_string_16digits(q);
  const __mmask16 mask = static_cast<__mmask16>(0xFFFFu << (16 - nq));
  _mm_mask_storeu_epi8(result - 16 + nq, mask, v16);
  write_four_digits(result + nq, r);
  return n;
}

simditoa_really_inline size_t to_chars_heterogeneous_unsigned(uint64_t value,
                                                              char *result) {
  if (value < 100000000ULL) {
    return static_cast<size_t>(het_to_chars_small(value, result));
  }
  if (value < 10000000000000000ULL) {
    return static_cast<size_t>(het_to_chars_medium(value, result));
  }
  return static_cast<size_t>(het_to_chars_large(value, result));
}

// =====================================================================
// Homogeneous variant (§5.5)
// =====================================================================
//
// Branch-heavy: a 20-way dispatch on the exact digit count, with each
// path using direct (unmasked) stores at compile-time offsets. When most
// inputs in a batch share the same digit length, the dispatcher branches
// are nearly free and the per-path bodies are tight straight-line code.

// Scalar writers for exactly N digits in [0, 10^N). Each is specialized
// so the compiler emits straight-line code rather than dynamic-length
// dispatch.
template <int N>
simditoa_really_inline void write_exactly_n(char *buf, uint64_t value);

template <>
simditoa_really_inline void write_exactly_n<1>(char *buf, uint64_t value) {
  buf[0] = static_cast<char>('0' + value);
}
template <>
simditoa_really_inline void write_exactly_n<2>(char *buf, uint64_t value) {
  write_two_digits(buf, value);
}
template <>
simditoa_really_inline void write_exactly_n<3>(char *buf, uint64_t value) {
  auto [q, r] = div100v(value);
  buf[0] = static_cast<char>('0' + q);
  write_two_digits_v(buf + 1, r);
}
template <>
simditoa_really_inline void write_exactly_n<4>(char *buf, uint64_t value) {
  write_four_digits(buf, value);
}

// 5-8 digit case: 8-digit SIMD kernel + masked store with constant mask
// and offset known at the call site.
template <int N>
simditoa_really_inline void hom_store_5to8(uint64_t value, char *result) {
  static_assert(N >= 5 && N <= 8, "hom_store_5to8 expects N in [5, 8]");
  const __m512i digits = to_string_8digits(value);
  constexpr __mmask8 mask = static_cast<__mmask8>(0xff00u >> N);
  _mm512_mask_cvtusepi64_storeu_epi8(result - 8 + N, mask, digits);
}

// 9-15 digit case: 16-digit SIMD kernel, shift away the (16 - N) leading
// zero bytes, direct unmasked store. The store also writes (16 - N)
// trailing zero bytes after the digits; callers use the returned length
// to know where the string ends.
template <int N>
simditoa_really_inline void hom_store_9to15(uint64_t value, char *result) {
  static_assert(N >= 9 && N <= 15, "hom_store_9to15 expects N in [9, 15]");
  const __m128i digits = to_string_16digits(value);
  const __m128i shifted = _mm_bsrli_si128(digits, 16 - N);
  _mm_storeu_si128(reinterpret_cast<__m128i *>(result), shifted);
}

// 17-20 digit case: scalar prefix (1-4 digits) followed by a 16-digit
// SIMD block written with a direct unmasked store.
template <int N>
simditoa_really_inline void hom_store_17to20(uint64_t value, char *result) {
  static_assert(N >= 17 && N <= 20, "hom_store_17to20 expects N in [17, 20]");
  auto [q, r] = div10e16(value);
  write_exactly_n<N - 16>(result, q);
  const __m128i digits = to_string_16digits(r);
  _mm_storeu_si128(reinterpret_cast<__m128i *>(result + (N - 16)), digits);
}

simditoa_really_inline size_t to_chars_homogeneous_unsigned(uint64_t value,
                                                            char *result) {
  const int n = fast_digit_count(value);
  switch (n) {
  case 1:
    write_exactly_n<1>(result, value);
    return 1;
  case 2:
    write_exactly_n<2>(result, value);
    return 2;
  case 3:
    write_exactly_n<3>(result, value);
    return 3;
  case 4:
    write_exactly_n<4>(result, value);
    return 4;
  case 5:
    hom_store_5to8<5>(value, result);
    return 5;
  case 6:
    hom_store_5to8<6>(value, result);
    return 6;
  case 7:
    hom_store_5to8<7>(value, result);
    return 7;
  case 8:
    hom_store_5to8<8>(value, result);
    return 8;
  case 9:
    hom_store_9to15<9>(value, result);
    return 9;
  case 10:
    hom_store_9to15<10>(value, result);
    return 10;
  case 11:
    hom_store_9to15<11>(value, result);
    return 11;
  case 12:
    hom_store_9to15<12>(value, result);
    return 12;
  case 13:
    hom_store_9to15<13>(value, result);
    return 13;
  case 14:
    hom_store_9to15<14>(value, result);
    return 14;
  case 15:
    hom_store_9to15<15>(value, result);
    return 15;
  case 16: {
    const __m128i digits = to_string_16digits(value);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(result), digits);
    return 16;
  }
  case 17:
    hom_store_17to20<17>(value, result);
    return 17;
  case 18:
    hom_store_17to20<18>(value, result);
    return 18;
  case 19:
    hom_store_17to20<19>(value, result);
    return 19;
  case 20:
    hom_store_17to20<20>(value, result);
    return 20;
  }
  __builtin_unreachable();
}

// =====================================================================
// Variant entry points
// =====================================================================

size_t to_chars_heterogeneous(uint64_t value, char *buffer) noexcept {
  return to_chars_heterogeneous_unsigned(value, buffer);
}

size_t to_chars_heterogeneous(int64_t value, char *buffer) noexcept {
  if (value < 0) {
    *buffer++ = '-';
    uint64_t uval = static_cast<uint64_t>(-(value + 1)) + 1;
    return 1 + to_chars_heterogeneous_unsigned(uval, buffer);
  }
  return to_chars_heterogeneous_unsigned(static_cast<uint64_t>(value), buffer);
}

size_t to_chars_homogeneous(uint64_t value, char *buffer) noexcept {
  return to_chars_homogeneous_unsigned(value, buffer);
}

size_t to_chars_homogeneous(int64_t value, char *buffer) noexcept {
  if (value < 0) {
    *buffer++ = '-';
    uint64_t uval = static_cast<uint64_t>(-(value + 1)) + 1;
    return 1 + to_chars_homogeneous_unsigned(uval, buffer);
  }
  return to_chars_homogeneous_unsigned(static_cast<uint64_t>(value), buffer);
}

// Default: heterogeneous (matches the prior single-call behavior).
size_t to_chars(int64_t value, char *buffer) noexcept {
  return to_chars_heterogeneous(value, buffer);
}

size_t to_chars(uint64_t value, char *buffer) noexcept {
  return to_chars_heterogeneous(value, buffer);
}

} // namespace avx512
} // namespace simditoa

#endif // SIMDITOA_HAS_AVX512_IFMA

#endif // SIMDITOA_SRC_AVX512_CPP
