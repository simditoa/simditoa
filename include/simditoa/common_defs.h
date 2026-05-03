#ifndef SIMDITOA_COMMON_DEFS_H
#define SIMDITOA_COMMON_DEFS_H

#include <cassert>
#include "simditoa/portability.h"

#if SIMDITOA_REGULAR_VISUAL_STUDIO
  #define simditoa_really_inline __forceinline
  #define simditoa_never_inline __declspec(noinline)

  #define simditoa_unused
  #define simditoa_warn_unused

  #ifndef simditoa_likely
    #define simditoa_likely(x) x
  #endif
  #ifndef simditoa_unlikely
    #define simditoa_unlikely(x) x
  #endif

  #define SIMDITOA_PUSH_DISABLE_WARNINGS __pragma(warning(push))
  #define SIMDITOA_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0))
  #define SIMDITOA_POP_DISABLE_WARNINGS __pragma(warning(pop))
  #define SIMDITOA_DISABLE_VS_WARNING(WARNING_NUMBER)                          \
    __pragma(warning(disable : WARNING_NUMBER))

  #define SIMDITOA_PUSH_DISABLE_UNUSED_WARNINGS SIMDITOA_PUSH_DISABLE_WARNINGS
  #define SIMDITOA_POP_DISABLE_UNUSED_WARNINGS SIMDITOA_POP_DISABLE_WARNINGS
#else
  #define simditoa_really_inline inline __attribute__((always_inline))
  #define simditoa_never_inline inline __attribute__((noinline))

  #define simditoa_unused __attribute__((unused))
  #define simditoa_warn_unused __attribute__((warn_unused_result))

  #ifndef simditoa_likely
    #define simditoa_likely(x) __builtin_expect(!!(x), 1)
  #endif
  #ifndef simditoa_unlikely
    #define simditoa_unlikely(x) __builtin_expect(!!(x), 0)
  #endif

  #define SIMDITOA_PUSH_DISABLE_WARNINGS _Pragma("GCC diagnostic push")
  #define SIMDITOA_PUSH_DISABLE_ALL_WARNINGS                                   \
    SIMDITOA_PUSH_DISABLE_WARNINGS                                             \
    _Pragma("GCC diagnostic ignored \"-Wall\"")
  #define SIMDITOA_POP_DISABLE_WARNINGS _Pragma("GCC diagnostic pop")

  #define SIMDITOA_PUSH_DISABLE_UNUSED_WARNINGS                                \
    SIMDITOA_PUSH_DISABLE_WARNINGS                                             \
    _Pragma("GCC diagnostic ignored \"-Wunused\"")
  #define SIMDITOA_POP_DISABLE_UNUSED_WARNINGS SIMDITOA_POP_DISABLE_WARNINGS
#endif

#define SIMDITOA_ROUNDUP_N(a, n) (((a) + ((n) - 1)) & ~((n) - 1))

#endif // SIMDITOA_COMMON_DEFS_H
