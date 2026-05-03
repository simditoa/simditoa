#define SIMDITOA_SRC_SIMDITOA_CPP

#include "base.h"

SIMDITOA_PUSH_DISABLE_UNUSED_WARNINGS

// Fallback is always compiled; other implementations may delegate to it.
#include "fallback.cpp"

#define SIMDITOA_CONDITIONAL_INCLUDE

#if SIMDITOA_IMPLEMENTATION_AVX512 && SIMDITOA_HAS_AVX512_IFMA
  #include "avx512.cpp"
#endif

#undef SIMDITOA_CONDITIONAL_INCLUDE

// Public API — dispatches to the builtin implementation
namespace simditoa {

size_t to_chars(int64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars(value, buffer);
}

size_t to_chars(uint64_t value, char *buffer) noexcept {
  return SIMDITOA_BUILTIN_IMPLEMENTATION::to_chars(value, buffer);
}

} // namespace simditoa

SIMDITOA_POP_DISABLE_UNUSED_WARNINGS
