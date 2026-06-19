#ifndef SIMDITOA_PORTABILITY_H
#define SIMDITOA_PORTABILITY_H

#include "simditoa/compiler_check.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <climits>

static_assert(CHAR_BIT == 8, "simditoa requires 8-bit bytes");

using std::size_t;

#ifdef _MSC_VER
  #define SIMDITOA_VISUAL_STUDIO 1
  #ifdef __clang__
    #define SIMDITOA_CLANG_VISUAL_STUDIO 1
  #else
    #define SIMDITOA_REGULAR_VISUAL_STUDIO 1
  #endif
#endif

#if (defined(__x86_64__) || defined(_M_AMD64)) && !defined(_M_ARM64EC) &&      \
    !defined(__riscv)
  #define SIMDITOA_IS_X86_64 1
#endif

#if defined(__riscv)
  #define SIMDITOA_IS_RISCV 1
  #if defined(__riscv_xlen) && (__riscv_xlen == 64)
    #define SIMDITOA_IS_RISCV64 1
  #endif
#endif

// Ensure macros are always defined (0 if not detected)
#ifndef SIMDITOA_IS_X86_64
  #define SIMDITOA_IS_X86_64 0
#endif
#ifndef SIMDITOA_IS_RISCV
  #define SIMDITOA_IS_RISCV 0
#endif
#ifndef SIMDITOA_IS_RISCV64
  #define SIMDITOA_IS_RISCV64 0
#endif

// AVX-512 IFMA + VBMI detection (required for Champagne-Lemire algorithm)
#if defined(__AVX512IFMA__) && defined(__AVX512VBMI__)
  #define SIMDITOA_HAS_AVX512_IFMA 1
#else
  #define SIMDITOA_HAS_AVX512_IFMA 0
#endif

// Compiler support for AVX-512 target attribute (runtime dispatch)
#ifndef SIMDITOA_AVX512_ALLOWED
  #define SIMDITOA_AVX512_ALLOWED 1
#endif

// Can the compiler generate AVX-512 IFMA code (even if not natively enabled)?
#if SIMDITOA_IS_X86_64 && SIMDITOA_AVX512_ALLOWED &&                           \
    !defined(SIMDITOA_REGULAR_VISUAL_STUDIO)
  #define SIMDITOA_COMPILER_SUPPORTS_AVX512_IFMA 1
#else
  #define SIMDITOA_COMPILER_SUPPORTS_AVX512_IFMA 0
#endif

#endif // SIMDITOA_PORTABILITY_H
